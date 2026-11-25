#include "parser.h"
#include <set>
#include <cstdio>
#include <stdexcept>

struct Sym {
    int id;
    string name, kind, type, owner;
    int scope, slot, line;
};
struct Scope {
    int id, parent;
    map<string, int> names;
};
struct FieldD {
    string name, type;
    int owner, offset;
};
struct MethodD {
    string name, ret;
    vector<string> params;
    int owner, slot;
};
struct ClassD {
    string name;
    int parent;
    vector<FieldD> fields;
    vector<MethodD> methods;
};
struct Ctx {
    int classId;
    string ret;
    int loopDepth;
    bool inMethod;
    string methodName;
};
struct Flow {
    set<int> facts;
    bool reachable;
};

static set<int> assigned;

static vector<Sym> symbols;
static vector<Scope> scopes;
static vector<ClassD> classes;
static vector<Node*> classNodes;
static map<string, int> classIds;
static vector<string> diags;
static int symCount = 0, scopeCount = 0, curSlot = 0;

static void report(int line, const string& msg) {
    diags.push_back("Semantic error (line " + to_string(line) + "): " + msg);
}

static bool isIntType(const string& t) {
    return t == "int16" || t == "int32" || t == "uint16" || t == "uint32";
}
static string internalType(const string& t) {
    if (isIntType(t)) return "integer";
    return t;
}
static bool isTruthy(const string& t) {
    return t == "bool" || isIntType(t);
}
static bool rangeOk(const string& t, long long v) {
    if (t == "int16") return v >= -32768 && v <= 32767;
    if (t == "uint16") return v >= 0 && v <= 65535;
    if (t == "int32") return v >= -2147483648LL && v <= 2147483647LL;
    if (t == "uint32") return v >= 0 && v <= 4294967295LL;
    return true;
}
static long long charCode(const string& lex) {
    if (lex.length() >= 4 && lex[1] == '\\') {
        char e = lex[2];
        if (e == 'n') return 10;
        if (e == 't') return 9;
        if (e == 'r') return 13;
        if (e == '0') return 0;
        if (e == '\\') return 92;
        if (e == '\'') return 39;
    }
    return (long long)(unsigned char)lex[1];
}

static bool constEval(Node* n, long long& out) {
    if (n->kind == "Int") {
        try {
            out = stoll(n->text);
        }
        catch (...) {
            return false;
        }
        return true;
    }
    if (n->kind == "Char") {
        out = charCode(n->text);
        return true;
    }
    if (n->kind == "Unary" && n->text == "-") {
        if (constEval(n->kids[0], out)) {
            out = -out;
            return true;
        }
        return false;
    }
    if (n->kind == "Binary") {
        string op = n->text;
        if (op == "+" || op == "-" || op == "*" || op == "/") {
            long long a, b;
            if (constEval(n->kids[0], a) && constEval(n->kids[1], b)) {
                if (op == "+") out = a + b;
                else if (op == "-") out = a - b;
                else if (op == "*") out = a * b;
                else {
                    if (b == 0) {
                        report(n->line, "division by constant zero");
                        return false;
                    }
                    out = a / b;
                }
                return true;
            }
        }
    }
    return false;
}

static int newScope(int parent) {
    Scope s;
    s.id = scopeCount;
    s.parent = parent;
    scopes.push_back(s);
    return scopeCount++;
}

static int declare(int scopeIdx, const string& name, const string& kind, const string& type, const string& owner, int line) {
    if (scopes[scopeIdx].names.count(name)) {
        report(line, "duplicate declaration of '" + name + "' in the same scope");
        return -1;
    }
    Sym s;
    s.id = symCount++;
    s.name = name;
    s.kind = kind;
    s.type = type;
    s.owner = owner;
    s.scope = scopeIdx;
    s.slot = curSlot;
    s.line = line;
    curSlot++;
    scopes[scopeIdx].names[name] = s.id;
    symbols.push_back(s);
    return s.id;
}

static int lookup(int scopeIdx, const string& name) {
    int s = scopeIdx;
    while (s != -1) {
        if (scopes[s].names.count(name)) return scopes[s].names[name];
        s = scopes[s].parent;
    }
    return -1;
}

static int findField(int cid, const string& name, FieldD& out) {
    for (int i = 0; i < (int)classes[cid].fields.size(); i++) {
        if (classes[cid].fields[i].name == name) {
            out = classes[cid].fields[i];
            return i;
        }
    }
    return -1;
}
static int findMethod(int cid, const string& name, MethodD& out) {
    for (int i = 0; i < (int)classes[cid].methods.size(); i++) {
        if (classes[cid].methods[i].name == name) {
            out = classes[cid].methods[i];
            return i;
        }
    }
    return -1;
}
static bool hasFieldName(int cid, const string& name) {
    FieldD tmp;
    return findField(cid, name, tmp) != -1;
}
static bool hasMethodName(int cid, const string& name) {
    MethodD tmp;
    return findMethod(cid, name, tmp) != -1;
}
static vector<string> methodParams(Node* m) {
    vector<string> p;
    for (int i = 0; i < (int)m->kids.size(); i++) {
        if (m->kids[i]->kind == "Param") p.push_back(m->kids[i]->type);
    }
    return p;
}
static bool objCompat(const string& src, const string& dst) {
    if (classIds.find(src.substr(7)) == classIds.end()) return false;
    if (classIds.find(dst.substr(7)) == classIds.end()) return false;
    int s = classIds[src.substr(7)];
    int d = classIds[dst.substr(7)];
    int c = s;
    while (c != -1) {
        if (c == d) return true;
        c = classes[c].parent;
    }
    return false;
}

static vector<int> classState;
static MethodD makeMethod(Node* m, int owner, int slot) {
    MethodD d;
    d.name = m->text;
    d.ret = m->type;
    d.params = methodParams(m);
    d.owner = owner;
    d.slot = slot;
    return d;
}
static bool layoutClass(int i) {
    if (classState[i] == 2) return true;
    if (classState[i] == 1) {
        report(classNodes[i]->line, "inheritance cycle involving class '" + classes[i].name + "'");
        return false;
    }
    classState[i] = 1;
    bool ok = true;
    if (classes[i].parent != -1) ok = layoutClass(classes[i].parent);
    classState[i] = 2;
    if (!ok) return false;
    if (classes[i].parent != -1) {
        classes[i].fields = classes[classes[i].parent].fields;
        classes[i].methods = classes[classes[i].parent].methods;
    }
    Node* cn = classNodes[i];
    for (int k = 0; k < (int)cn->kids.size(); k++) {
        Node* m = cn->kids[k];
        if (m->kind == "Field") {
            if (hasFieldName(i, m->text) || hasMethodName(i, m->text)) {
                report(m->line, "duplicate member or field hiding for '" + m->text + "' in class '" + classes[i].name + "'");
                continue;
            }
            FieldD f;
            f.name = m->text;
            f.type = m->type;
            f.owner = i;
            f.offset = (int)classes[i].fields.size();
            classes[i].fields.push_back(f);
        }
        else {
            int mi = -1;
            MethodD tmp;
            for (int j = 0; j < (int)classes[i].methods.size(); j++) {
                if (classes[i].methods[j].name == m->text) {
                    mi = j;
                    tmp = classes[i].methods[j];
                    break;
                }
            }
            if (mi != -1) {
                if (tmp.ret != m->type || tmp.params != methodParams(m)) {
                    report(m->line, "override of '" + m->text + "' must match the inherited signature");
                    continue;
                }
                MethodD d = makeMethod(m, i, tmp.slot);
                classes[i].methods[mi] = d;
            }
            else {
                if (hasFieldName(i, m->text)) {
                    report(m->line, "method '" + m->text + "' collides with a field in class '" + classes[i].name + "'");
                    continue;
                }
                MethodD d = makeMethod(m, i, (int)classes[i].methods.size());
                classes[i].methods.push_back(d);
            }
        }
    }
    for (int k = 0; k < (int)classes[i].fields.size(); k++) {
        FieldD f = classes[i].fields[k];
        Sym s;
        s.id = symCount++;
        s.name = f.name;
        s.kind = "field";
        s.type = f.type;
        s.owner = "c" + to_string(i);
        s.scope = -1;
        s.slot = k;
        s.line = classNodes[i]->line;
        symbols.push_back(s);
    }
    for (int k = 0; k < (int)classes[i].methods.size(); k++) {
        MethodD d = classes[i].methods[k];
        Sym s;
        s.id = symCount++;
        s.name = d.name;
        s.kind = "method";
        s.type = d.ret;
        s.owner = "c" + to_string(i);
        s.scope = -1;
        s.slot = d.slot;
        s.line = classNodes[i]->line;
        symbols.push_back(s);
    }
    return true;
}

static void collectClasses(Node* prog) {
    for (int i = 0; i < (int)prog->kids.size(); i++) {
        Node* k = prog->kids[i];
        if (k->kind != "Class") continue;
        if (classIds.count(k->text)) {
            report(k->line, "duplicate class '" + k->text + "'");
            continue;
        }
        classIds[k->text] = (int)classes.size();
        ClassD c;
        c.name = k->text;
        c.parent = -1;
        classes.push_back(c);
        classNodes.push_back(k);
    }
    for (int i = 0; i < (int)classNodes.size(); i++) {
        string p = classNodes[i]->type;
        if (p == "") continue;
        if (classIds.find(p) == classIds.end()) {
            report(classNodes[i]->line, "unknown parent class '" + p + "'");
        }
        else {
            classes[i].parent = classIds[p];
        }
    }
    classState.assign(classes.size(), 0);
    for (int i = 0; i < (int)classes.size(); i++) {
        layoutClass(i);
    }
}

static bool checkAssign(const string& dst, const string& src, Node* value, int line) {
    if (dst == "error" || src == "error") return true;
    if (isIntType(dst)) {
        if (src == "integer") {
            long long v;
            if (constEval(value, v) && !rangeOk(dst, v)) {
                report(line, "constant " + to_string(v) + " is out of range for " + dst);
                return false;
            }
            return true;
        }
        if (src == "bool") report(line, "cannot assign bool to " + dst);
        else if (src == "char") report(line, "cannot assign char to " + dst + " (no implicit char-to-int conversion)");
        else report(line, "cannot assign " + src + " to " + dst);
        return false;
    }
    if (dst == "char") {
        if (src == "char") return true;
        if (src == "integer") report(line, "cannot assign integer to char, use a character literal");
        else if (src == "bool") report(line, "cannot assign bool to char");
        else report(line, "cannot assign object reference to char");
        return false;
    }
    if (dst.length() > 7 && dst.substr(0, 7) == "object:") {
        if (src.length() > 7 && src.substr(0, 7) == "object:" && objCompat(src, dst)) return true;
        report(line, "cannot assign " + src + " to " + dst + " (target type must be the same class or an ancestor)");
        return false;
    }
    return true;
}

static string checkExpr(Node* n, int scopeIdx, Ctx ctx);

static string checkExpr(Node* n, int scopeIdx, Ctx ctx) {
    if (n->kind == "Int") return "integer";
    if (n->kind == "Char") return "char";
    if (n->kind == "Var") {
        int id = lookup(scopeIdx, n->text);
        if (id == -1) {
            report(n->line, "undeclared variable '" + n->text + "'");
            return "error";
        }
        if (symbols[id].kind == "var" && assigned.find(id) == assigned.end()) {
            report(n->line, "read of possibly unassigned variable '" + n->text + "'");
        }
        return internalType(symbols[id].type);
    }
    if (n->kind == "This") {
        if (!ctx.inMethod) {
            report(n->line, "'this' used outside a method");
            return "error";
        }
        return "object:" + classes[ctx.classId].name;
    }
    if (n->kind == "New") {
        if (classIds.find(n->text) == classIds.end()) {
            report(n->line, "unknown class '" + n->text + "'");
            return "error";
        }
        return "object:" + n->text;
    }
    if (n->kind == "FieldAccess") {
        string rt = checkExpr(n->kids[0], scopeIdx, ctx);
        if (rt == "error") return "error";
        if (rt.length() <= 7 || rt.substr(0, 7) != "object:") {
            report(n->line, "member access on non-object value of type " + rt);
            return "error";
        }
        string cname = rt.substr(7);
        int cid = classIds[cname];
        FieldD f;
        if (findField(cid, n->text, f) == -1) {
            string what = hasMethodName(cid, n->text) ? "method '" + n->text + "' cannot be used as a field" : "class '" + cname + "' has no field '" + n->text + "'";
            report(n->line, what);
            return "error";
        }
        return internalType(f.type);
    }
    if (n->kind == "Call") {
        string rt = checkExpr(n->kids[0], scopeIdx, ctx);
        if (rt == "error") return "error";
        if (rt.length() <= 7 || rt.substr(0, 7) != "object:") {
            report(n->line, "method call on non-object value of type " + rt);
            return "error";
        }
        string cname = rt.substr(7);
        int cid = classIds[cname];
        MethodD m;
        if (findMethod(cid, n->text, m) == -1) {
            string what = hasFieldName(cid, n->text) ? "field '" + n->text + "' cannot be called" : "class '" + cname + "' has no method '" + n->text + "'";
            report(n->line, what);
            return "error";
        }
        vector<Node*> args;
        for (int i = 1; i < (int)n->kids.size(); i++) args.push_back(n->kids[i]);
        if (args.size() != m.params.size()) {
            report(n->line, "wrong argument count for method '" + n->text + "': expected " + to_string(m.params.size()) + ", got " + to_string(args.size()));
            for (int i = 0; i < (int)args.size(); i++) checkExpr(args[i], scopeIdx, ctx);
            return internalType(m.ret);
        }
        for (int i = 0; i < (int)args.size(); i++) {
            string at = checkExpr(args[i], scopeIdx, ctx);
            checkAssign(m.params[i], at, args[i], args[i]->line);
        }
        return internalType(m.ret);
    }
    if (n->kind == "Unary") {
        string t = checkExpr(n->kids[0], scopeIdx, ctx);
        if (t == "error") return "error";
        if (n->text == "-") {
            if (t == "integer") return "integer";
            report(n->line, "unary '-' requires an integer operand, got " + t);
            return "error";
        }
        if (isTruthy(t)) return "bool";
        report(n->line, "'not' requires a bool or integer operand, got " + t);
        return "error";
    }
    if (n->kind == "Binary") {
        string op = n->text;
        string lt = checkExpr(n->kids[0], scopeIdx, ctx);
        string rt = checkExpr(n->kids[1], scopeIdx, ctx);
        if (lt == "error" || rt == "error") return "error";
        if (op == "+" || op == "-" || op == "*" || op == "/") {
            if (lt == "integer" && rt == "integer") return "integer";
            report(n->line, "arithmetic '" + op + "' requires integer operands, got " + lt + " and " + rt);
            return "error";
        }
        if (op == "<" || op == "<=" || op == ">" || op == ">=") {
            if ((lt == "integer" && rt == "integer") || (lt == "char" && rt == "char")) return "bool";
            report(n->line, "ordering '" + op + "' requires two integers or two chars, got " + lt + " and " + rt);
            return "error";
        }
        if (op == "=" || op == "!=") {
            if (lt == "integer" && rt == "integer") return "bool";
            if (lt == "char" && rt == "char") return "bool";
            if (lt == "bool" && rt == "bool") return "bool";
            if (lt.length() > 7 && lt.substr(0, 7) == "object:" && rt.length() > 7 && rt.substr(0, 7) == "object:") {
                if (objCompat(lt, rt) || objCompat(rt, lt)) return "bool";
                report(n->line, "incompatible reference comparison between " + lt + " and " + rt);
                return "error";
            }
            report(n->line, "cannot compare " + lt + " and " + rt + " with '" + op + "'");
            return "error";
        }
        if (op == "and" || op == "or") {
            if (isTruthy(lt) && isTruthy(rt)) return "bool";
            report(n->line, "'" + op + "' requires bool or integer operands, got " + lt + " and " + rt);
            return "error";
        }
    }
    return "error";
}

static void checkCond(Node* n, int scopeIdx, Ctx ctx) {
    string t = checkExpr(n, scopeIdx, ctx);
    if (t != "error" && !isTruthy(t)) {
        report(n->line, "condition must be bool or integer, got " + t);
    }
}

static void checkBlock(Node* block, int parentScope, Ctx ctx, const Flow& in, Flow& out);

static void checkAssignNode(Node* n, int scopeIdx, Ctx ctx, const Flow& in, Flow& out) {
    assigned = in.facts;
    out = in;
    Node* tgt = n->kids[0];
    Node* val = n->kids[1];
    if (tgt->kind == "This") {
        report(n->line, "'this' is not assignable");
        checkExpr(val, scopeIdx, ctx);
        return;
    }
    if (tgt->kind == "Var") {
        int id = lookup(scopeIdx, tgt->text);
        if (id == -1) {
            report(tgt->line, "undeclared variable '" + tgt->text + "'");
            checkExpr(val, scopeIdx, ctx);
            return;
        }
        string vt = checkExpr(val, scopeIdx, ctx);
        if (checkAssign(symbols[id].type, vt, val, n->line)) {
            out.facts.insert(id);
        }
        return;
    }
    string rt = checkExpr(tgt->kids[0], scopeIdx, ctx);
    string vt = checkExpr(val, scopeIdx, ctx);
    if (rt == "error" || vt == "error") return;
    if (rt.length() <= 7 || rt.substr(0, 7) != "object:") {
        report(n->line, "field assignment on non-object value of type " + rt);
        return;
    }
    int cid = classIds[rt.substr(7)];
    FieldD f;
    if (findField(cid, tgt->text, f) == -1) {
        report(tgt->line, "class '" + rt.substr(7) + "' has no field '" + tgt->text + "'");
        return;
    }
    checkAssign(f.type, vt, val, n->line);
}

static void checkStmt(Node* n, int scopeIdx, Ctx ctx, const Flow& in, Flow& out) {
    assigned = in.facts;
    out = in;
    if (n->kind == "Decl") {
        if (n->type.length() > 7 && n->type.substr(0, 7) == "object:") {
            string cname = n->type.substr(7);
            if (classIds.find(cname) == classIds.end()) {
                report(n->line, "unknown class '" + cname + "' in declaration of '" + n->text + "'");
            }
        }
        declare(scopeIdx, n->text, "var", n->type, ctx.inMethod ? ("c" + to_string(ctx.classId) + "." + ctx.methodName) : "main", n->line);
    }
    else if (n->kind == "Assign") {
        checkAssignNode(n, scopeIdx, ctx, in, out);
    }
    else if (n->kind == "While") {
        checkCond(n->kids[0], scopeIdx, ctx);
        Ctx bodyCtx = ctx;
        bodyCtx.loopDepth++;
        Flow bin = in;
        Flow bout;
        checkBlock(n->kids[1], scopeIdx, bodyCtx, bin, bout);
        out = in;
    }
    else if (n->kind == "For") {
        Flow finit;
        checkAssignNode(n->kids[0], scopeIdx, ctx, in, finit);
        assigned = finit.facts;
        checkCond(n->kids[1], scopeIdx, ctx);
        Ctx bodyCtx = ctx;
        bodyCtx.loopDepth++;
        Flow bin = finit;
        Flow bout;
        checkBlock(n->kids[3], scopeIdx, bodyCtx, bin, bout);
        Flow fupd;
        checkAssignNode(n->kids[2], scopeIdx, ctx, finit, fupd);
        out = finit;
    }
    else if (n->kind == "If") {
        vector<Flow> exits;
        bool hasElse = false;
        for (int i = 0; i < (int)n->kids.size(); i++) {
            Node* k = n->kids[i];
            if (k->kind == "Branch") {
                assigned = in.facts;
                checkCond(k->kids[0], scopeIdx, ctx);
                Flow bo;
                checkBlock(k->kids[1], scopeIdx, ctx, in, bo);
                if (bo.reachable) exits.push_back(bo);
            }
            else {
                Flow eo;
                checkBlock(k, scopeIdx, ctx, in, eo);
                if (eo.reachable) exits.push_back(eo);
                hasElse = true;
            }
        }
        if (!hasElse) exits.push_back(in);
        if (exits.empty()) {
            out.reachable = false;
            return;
        }
        out.reachable = true;
        set<int> inter = exits[0].facts;
        for (int i = 1; i < (int)exits.size(); i++) {
            for (set<int>::iterator it = inter.begin(); it != inter.end();) {
                if (exits[i].facts.find(*it) == exits[i].facts.end()) it = inter.erase(it);
                else ++it;
            }
        }
        out.facts = inter;
    }
    else if (n->kind == "Break") {
        if (ctx.loopDepth == 0) report(n->line, "break outside a loop");
        out.reachable = false;
    }
    else if (n->kind == "Continue") {
        if (ctx.loopDepth == 0) report(n->line, "continue outside a loop");
        out.reachable = false;
    }
    else if (n->kind == "Return") {
        if (!ctx.inMethod) {
            report(n->line, "return outside a method");
            checkExpr(n->kids[0], scopeIdx, ctx);
            return;
        }
        string vt = checkExpr(n->kids[0], scopeIdx, ctx);
        checkAssign(ctx.ret, vt, n->kids[0], n->line);
        out.reachable = false;
    }
    else if (n->kind == "Print") {
        checkExpr(n->kids[0], scopeIdx, ctx);
        out = in;
    }
    else if (n->kind == "Block") {
        checkBlock(n, scopeIdx, ctx, in, out);
    }
}

static void checkBlock(Node* block, int parentScope, Ctx ctx, const Flow& in, Flow& out) {
    int sc = newScope(parentScope);
    Flow cur = in;
    for (int i = 0; i < (int)block->kids.size(); i++) {
        Flow so;
        checkStmt(block->kids[i], sc, ctx, cur, so);
        cur = so;
    }
    out = cur;
}

static bool definitelyReturns(Node* n) {
    if (n->kind == "Return") return true;
    if (n->kind == "Block") {
        for (int i = 0; i < (int)n->kids.size(); i++) {
            if (definitelyReturns(n->kids[i])) return true;
        }
        return false;
    }
    if (n->kind == "If") {
        bool hasElse = false;
        for (int i = 0; i < (int)n->kids.size(); i++) {
            Node* k = n->kids[i];
            if (k->kind == "Branch") {
                if (!definitelyReturns(k->kids[1])) return false;
            }
            else if (k->kind == "Block") {
                if (!definitelyReturns(k)) return false;
                hasElse = true;
            }
        }
        return hasElse;
    }
    return false;
}

static void checkMethod(Node* m, int cid) {
    Ctx ctx;
    ctx.classId = cid;
    ctx.ret = m->type;
    ctx.loopDepth = 0;
    ctx.inMethod = true;
    ctx.methodName = m->text;
    int sc = newScope(-1);
    curSlot = 0;
    Flow in;
    in.reachable = true;
    for (int i = 0; i < (int)m->kids.size(); i++) {
        Node* k = m->kids[i];
        if (k->kind == "Param") {
            int id = declare(sc, k->text, "param", k->type, "c" + to_string(cid) + "." + m->text, k->line);
            if (id != -1) in.facts.insert(id);
        }
    }
    Flow out;
    checkBlock(m->kids[m->kids.size() - 1], sc, ctx, in, out);
    if (!definitelyReturns(m->kids[m->kids.size() - 1])) {
        report(m->line, "method '" + m->text + "' may finish without returning a value");
    }
}

static void printSymbols() {
    cout << "SYMBOL TABLE" << endl;
    cout << "id  name       kind    type            scope  owner     slot  line" << endl;
    for (int i = 0; i < (int)symbols.size(); i++) {
        Sym s = symbols[i];
        string sc = (s.scope == -1) ? "-" : ("scope" + to_string(s.scope));
        printf("%-3d %-10s %-7s %-15s %-6s %-9s %-5d %d\n", s.id, s.name.c_str(), s.kind.c_str(), s.type.c_str(), sc.c_str(), s.owner.c_str(), s.slot, s.line);
    }
}

static void printClasses() {
    cout << endl;
    cout << "CLASS TABLE" << endl;
    for (int i = 0; i < (int)classes.size(); i++) {
        cout << "c" << i << " " << classes[i].name << " parent=";
        if (classes[i].parent == -1) cout << "-";
        else cout << "c" << classes[i].parent;
        cout << " fields=[";
        for (int j = 0; j < (int)classes[i].fields.size(); j++) {
            if (j > 0) cout << ", ";
            cout << classes[i].fields[j].name << ":" << classes[i].fields[j].type << "@" << classes[i].fields[j].offset;
        }
        cout << "] methods=[";
        for (int j = 0; j < (int)classes[i].methods.size(); j++) {
            if (j > 0) cout << ", ";
            MethodD d = classes[i].methods[j];
            cout << d.name << "(";
            for (int p = 0; p < (int)d.params.size(); p++) {
                if (p > 0) cout << ",";
                cout << d.params[p];
            }
            cout << "):" << d.ret << "@v" << d.slot;
        }
        cout << "]" << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "usage: semantic_analyzer <program.citu>" << endl;
        return 1;
    }
    ifstream input(argv[1]);
    if (!input) {
        cout << "Could not open input file: " << argv[1] << endl;
        return 1;
    }
    string code((istreambuf_iterator<char>(input)), istreambuf_iterator<char>());
    string lexError;
    vector<Token> tokens = tokenize(code, lexError);
    if (lexError != "") {
        cout << lexError << endl;
        cout << "Lexical analysis failed." << endl;
        return 1;
    }
    string error;
    int errLine = 0, errCol = 0;
    PNode* root = parseTokens(tokens, error, errLine, errCol);
    if (root == NULL) {
        cout << "Syntax error at line " << errLine << ", column " << errCol << ": " << error << endl;
        cout << "Parsing failed." << endl;
        return 1;
    }
    Node* ast = buildAst(root);
    collectClasses(ast);
    for (int i = 0; i < (int)ast->kids.size(); i++) {
        Node* k = ast->kids[i];
        if (k->kind != "Class") continue;
        for (int j = 0; j < (int)k->kids.size(); j++) {
            if (k->kids[j]->kind == "Method") checkMethod(k->kids[j], classIds[k->text]);
        }
    }
    Ctx mainCtx;
    mainCtx.classId = -1;
    mainCtx.ret = "";
    mainCtx.loopDepth = 0;
    mainCtx.inMethod = false;
    mainCtx.methodName = "";
    int mainScope = newScope(-1);
    curSlot = 0;
    Flow mainIn;
    mainIn.reachable = true;
    Flow mainOut;
    for (int i = 0; i < (int)ast->kids.size(); i++) {
        Node* k = ast->kids[i];
        if (k->kind != "Class") {
            checkStmt(k, mainScope, mainCtx, mainIn, mainOut);
            mainIn = mainOut;
        }
    }
    printSymbols();
    printClasses();
    cout << endl;
    if (diags.empty()) {
        cout << "Semantic analysis passed." << endl;
    }
    else {
        for (int i = 0; i < (int)diags.size(); i++) {
            cout << diags[i] << endl;
        }
        cout << "Semantic analysis failed with " << diags.size() << " error(s)." << endl;
    }
    return diags.empty() ? 0 : 1;
}
