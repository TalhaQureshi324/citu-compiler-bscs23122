#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"
#include <map>
#include <set>

struct Node {
    int id;
    int line;
    string kind;
    string text;
    string type;
    vector<Node*> kids;
    Node(string k) { id = 0; line = 0; kind = k; }
    Node(string k, string t) { id = 0; line = 0; kind = k; text = t; }
};

struct PNode {
    string sym;
    string lex;
    int line;
    vector<PNode*> kids;
    PNode(string s) { sym = s; lex = ""; line = 0; }
};

static vector<string> LHS;
static vector<vector<string> > RHS;
static map<pair<string, string>, int> M;
static set<string> NONTERMS;

static void addProd(const string& lhs, vector<string> rhs) {
    LHS.push_back(lhs);
    RHS.push_back(rhs);
    NONTERMS.insert(lhs);
}

static vector<string> setT() {
    vector<string> v;
    v.push_back("int16"); v.push_back("int32"); v.push_back("uint16"); v.push_back("uint32");
    v.push_back("char"); v.push_back("object");
    return v;
}
static vector<string> setS() {
    vector<string> v = setT();
    v.push_back("ID"); v.push_back("this"); v.push_back("jabtak"); v.push_back("for");
    v.push_back("agr"); v.push_back("shuru"); v.push_back("return"); v.push_back("break");
    v.push_back("continue"); v.push_back("print");
    return v;
}
static vector<string> setQ() {
    vector<string> v = setS();
    v.push_back("EOF"); v.push_back("khtm");
    return v;
}
static vector<string> setO() {
    vector<string> v;
    v.push_back("ID"); v.push_back("this"); v.push_back("INT_LIT"); v.push_back("CHAR_LIT");
    v.push_back("("); v.push_back("new");
    return v;
}
static vector<string> setE() {
    vector<string> v = setO();
    v.push_back("-"); v.push_back("not");
    return v;
}
static vector<string> setR() {
    vector<string> v;
    v.push_back("="); v.push_back("!="); v.push_back("<"); v.push_back("<=");
    v.push_back(">"); v.push_back(">=");
    return v;
}
static vector<string> setX() {
    vector<string> v = setQ();
    v.push_back(")"); v.push_back(","); v.push_back(";");
    return v;
}
static vector<string> setA() {
    vector<string> v = setX();
    v.push_back("or");
    return v;
}
static vector<string> setB() {
    vector<string> v = setA();
    vector<string> r = setR();
    v.insert(v.end(), r.begin(), r.end());
    return v;
}
static vector<string> setU() {
    vector<string> v = setB();
    v.push_back("+"); v.push_back("-"); v.push_back("*"); v.push_back("/");
    return v;
}

static void addRow(const string& nt, vector<string> look, int prod) {
    for (int i = 0; i < (int)look.size(); i++) {
        M[make_pair(nt, look[i])] = prod;
    }
}

static void initGrammar() {
    if (!LHS.empty()) return;
    addProd("PROGRAM", {"CLASSES", "SLIST", "EOF"});
    addProd("CLASSES", {"CLASS", "CLASSES"});
    addProd("CLASSES", {});
    addProd("CLASS", {"class", "ID", "BASE", "CST_CLASS"});
    addProd("BASE", {"extends", "ID"});
    addProd("BASE", {});
    addProd("CST_CLASS", {"shuru", "MEMBERS", "khtm"});
    addProd("MEMBERS", {"MEMBER", "MEMBERS"});
    addProd("MEMBERS", {});
    addProd("MEMBER", {"TYPE", "ID"});
    addProd("MEMBER", {"method", "TYPE", "ID", "(", "PARAMS", ")", "CST"});
    addProd("TYPE", {"int16"});
    addProd("TYPE", {"int32"});
    addProd("TYPE", {"uint16"});
    addProd("TYPE", {"uint32"});
    addProd("TYPE", {"char"});
    addProd("TYPE", {"object", "ID"});
    addProd("PARAMS", {"TYPE", "ID", "PARAM_TAIL"});
    addProd("PARAMS", {});
    addProd("PARAM_TAIL", {",", "TYPE", "ID", "PARAM_TAIL"});
    addProd("PARAM_TAIL", {});
    addProd("SLIST", {"ST", "SLIST"});
    addProd("SLIST", {});
    addProd("ST", {"DS"});
    addProd("ST", {"AS"});
    addProd("ST", {"WS"});
    addProd("ST", {"FS"});
    addProd("ST", {"IS"});
    addProd("ST", {"CST"});
    addProd("ST", {"RS"});
    addProd("ST", {"BS"});
    addProd("ST", {"CS"});
    addProd("ST", {"PS"});
    addProd("DS", {"TYPE", "ID"});
    addProd("AS", {"TARGET", "medalo", "EX"});
    addProd("TARGET", {"ROOT", "PATH"});
    addProd("ROOT", {"ID"});
    addProd("ROOT", {"this"});
    addProd("PATH", {".", "ID", "PATH"});
    addProd("PATH", {});
    addProd("PS", {"print", "EX"});
    addProd("CST", {"shuru", "SLIST", "khtm"});
    addProd("WS", {"jabtak", "(", "EX", ")", "CST"});
    addProd("FS", {"for", "(", "AS", ";", "EX", ";", "AS", ")", "CST"});
    addProd("IS", {"agr", "(", "EX", ")", "CST", "IT"});
    addProd("IT", {"wrna", "(", "EX", ")", "CST", "IT"});
    addProd("IT", {"wrna_phir", "CST"});
    addProd("IT", {});
    addProd("RS", {"return", "EX"});
    addProd("BS", {"break"});
    addProd("CS", {"continue"});
    addProd("EX", {"OR_EXPR"});
    addProd("OR_EXPR", {"AND_EXPR", "OR_TAIL"});
    addProd("OR_TAIL", {"or", "AND_EXPR", "OR_TAIL"});
    addProd("OR_TAIL", {});
    addProd("AND_EXPR", {"REL_EXPR", "AND_TAIL"});
    addProd("AND_TAIL", {"and", "REL_EXPR", "AND_TAIL"});
    addProd("AND_TAIL", {});
    addProd("REL_EXPR", {"ADD", "REL_TAIL"});
    addProd("REL_TAIL", {"REL", "ADD"});
    addProd("REL_TAIL", {});
    addProd("REL", {"="});
    addProd("REL", {"!="});
    addProd("REL", {"<"});
    addProd("REL", {"<="});
    addProd("REL", {">"});
    addProd("REL", {">="});
    addProd("ADD", {"TERM", "ADD_TAIL"});
    addProd("ADD_TAIL", {"+", "TERM", "ADD_TAIL"});
    addProd("ADD_TAIL", {"-", "TERM", "ADD_TAIL"});
    addProd("ADD_TAIL", {});
    addProd("TERM", {"UNARY", "TERM_TAIL"});
    addProd("TERM_TAIL", {"*", "UNARY", "TERM_TAIL"});
    addProd("TERM_TAIL", {"/", "UNARY", "TERM_TAIL"});
    addProd("TERM_TAIL", {});
    addProd("UNARY", {"-", "UNARY"});
    addProd("UNARY", {"not", "UNARY"});
    addProd("UNARY", {"PRIMARY"});
    addProd("PRIMARY", {"ATOM", "POST"});
    addProd("ATOM", {"ID"});
    addProd("ATOM", {"this"});
    addProd("ATOM", {"INT_LIT"});
    addProd("ATOM", {"CHAR_LIT"});
    addProd("ATOM", {"(", "EX", ")"});
    addProd("ATOM", {"new", "ID", "(", ")"});
    addProd("POST", {".", "ID", "CALL_OPT", "POST"});
    addProd("POST", {});
    addProd("CALL_OPT", {"(", "ARGS", ")"});
    addProd("CALL_OPT", {});
    addProd("ARGS", {"EX", "ARG_TAIL"});
    addProd("ARGS", {});
    addProd("ARG_TAIL", {",", "EX", "ARG_TAIL"});
    addProd("ARG_TAIL", {});

    vector<string> S = setS();
    vector<string> Q = setQ();
    vector<string> T = setT();
    vector<string> O = setO();
    vector<string> E = setE();
    vector<string> R = setR();
    vector<string> X = setX();
    vector<string> A = setA();
    vector<string> B = setB();
    vector<string> U = setU();
    vector<string> Cset = setB();
    Cset.push_back("+");
    Cset.push_back("-");
    vector<string> shuruOnly; shuruOnly.push_back("shuru");
    vector<string> idThis; idThis.push_back("ID"); idThis.push_back("this");
    vector<string> classOnly; classOnly.push_back("class");
    vector<string> extendsOnly; extendsOnly.push_back("extends");
    vector<string> methodOnly; methodOnly.push_back("method");
    vector<string> khtmOnly; khtmOnly.push_back("khtm");
    vector<string> rparOnly; rparOnly.push_back(")");
    vector<string> commaOnly; commaOnly.push_back(",");
    vector<string> dotOnly; dotOnly.push_back(".");
    vector<string> medaloOnly; medaloOnly.push_back("medalo");
    vector<string> eofKhtm; eofKhtm.push_back("EOF"); eofKhtm.push_back("khtm");
    vector<string> idOnly; idOnly.push_back("ID");
    vector<string> thisOnly; thisOnly.push_back("this");
    vector<string> printOnly; printOnly.push_back("print");
    vector<string> jabtakOnly; jabtakOnly.push_back("jabtak");
    vector<string> forOnly; forOnly.push_back("for");
    vector<string> agrOnly; agrOnly.push_back("agr");
    vector<string> returnOnly; returnOnly.push_back("return");
    vector<string> breakOnly; breakOnly.push_back("break");
    vector<string> continueOnly; continueOnly.push_back("continue");
    vector<string> wrnaOnly; wrnaOnly.push_back("wrna");
    vector<string> wrnaPhirOnly; wrnaPhirOnly.push_back("wrna_phir");
    vector<string> orOnly; orOnly.push_back("or");
    vector<string> Xor; Xor = X; Xor.push_back("or");
    vector<string> andOnly; andOnly.push_back("and");
    vector<string> plusOnly; plusOnly.push_back("+");
    vector<string> minusOnly; minusOnly.push_back("-");
    vector<string> starOnly; starOnly.push_back("*");
    vector<string> slashOnly; slashOnly.push_back("/");
    vector<string> dotU; dotU.push_back("."); 
    vector<string> Udot; Udot = U; Udot.push_back(".");
    vector<string> lparOnly; lparOnly.push_back("(");
    vector<string> intLitOnly; intLitOnly.push_back("INT_LIT");
    vector<string> charLitOnly; charLitOnly.push_back("CHAR_LIT");
    vector<string> newOnly; newOnly.push_back("new");

    S.push_back("class"); S.push_back("EOF");
    addRow("PROGRAM", S, 1);
    addRow("CLASSES", classOnly, 2);
    S.pop_back(); S.pop_back();
    S.push_back("EOF");
    addRow("CLASSES", S, 3);
    addRow("CLASS", classOnly, 4);
    addRow("BASE", extendsOnly, 5);
    addRow("BASE", shuruOnly, 6);
    addRow("CST_CLASS", shuruOnly, 7);
    vector<string> Tm = T; Tm.push_back("method");
    addRow("MEMBERS", Tm, 8);
    addRow("MEMBERS", khtmOnly, 9);
    addRow("MEMBER", T, 10);
    addRow("MEMBER", methodOnly, 11);
    addRow("TYPE", {"int16"}, 12);
    addRow("TYPE", {"int32"}, 13);
    addRow("TYPE", {"uint16"}, 14);
    addRow("TYPE", {"uint32"}, 15);
    addRow("TYPE", {"char"}, 16);
    addRow("TYPE", {"object"}, 17);
    addRow("PARAMS", T, 18);
    addRow("PARAMS", rparOnly, 19);
    addRow("PARAM_TAIL", commaOnly, 20);
    addRow("PARAM_TAIL", rparOnly, 21);
    addRow("SLIST", setS(), 22);
    addRow("SLIST", eofKhtm, 23);
    addRow("ST", T, 24);
    addRow("ST", idThis, 25);
    addRow("ST", jabtakOnly, 26);
    addRow("ST", forOnly, 27);
    addRow("ST", agrOnly, 28);
    addRow("ST", shuruOnly, 29);
    addRow("ST", returnOnly, 30);
    addRow("ST", breakOnly, 31);
    addRow("ST", continueOnly, 32);
    addRow("ST", printOnly, 33);
    addRow("DS", T, 34);
    addRow("AS", idThis, 35);
    addRow("TARGET", idThis, 36);
    addRow("ROOT", idOnly, 37);
    addRow("ROOT", thisOnly, 38);
    addRow("PATH", dotOnly, 39);
    addRow("PATH", medaloOnly, 40);
    addRow("PS", printOnly, 41);
    addRow("CST", shuruOnly, 42);
    addRow("WS", jabtakOnly, 43);
    addRow("FS", forOnly, 44);
    addRow("IS", agrOnly, 45);
    addRow("IT", wrnaOnly, 46);
    addRow("IT", wrnaPhirOnly, 47);
    addRow("IT", Q, 48);
    addRow("RS", returnOnly, 49);
    addRow("BS", breakOnly, 50);
    addRow("CS", continueOnly, 51);
    addRow("EX", E, 52);
    addRow("OR_EXPR", E, 53);
    addRow("OR_TAIL", orOnly, 54);
    addRow("OR_TAIL", X, 55);
    addRow("AND_EXPR", E, 56);
    addRow("AND_TAIL", andOnly, 57);
    addRow("AND_TAIL", Xor, 58);
    addRow("REL_EXPR", E, 59);
    addRow("REL_TAIL", R, 60);
    addRow("REL_TAIL", A, 61);
    addRow("REL", {"="}, 62);
    addRow("REL", {"!="}, 63);
    addRow("REL", {"<"}, 64);
    addRow("REL", {"<="}, 65);
    addRow("REL", {">"}, 66);
    addRow("REL", {">="}, 67);
    addRow("ADD", E, 68);
    addRow("ADD_TAIL", plusOnly, 69);
    addRow("ADD_TAIL", minusOnly, 70);
    addRow("ADD_TAIL", B, 71);
    addRow("TERM", E, 72);
    addRow("TERM_TAIL", starOnly, 73);
    addRow("TERM_TAIL", slashOnly, 74);
    addRow("TERM_TAIL", Cset, 75);
    addRow("UNARY", minusOnly, 76);
    addRow("UNARY", {"not"}, 77);
    addRow("UNARY", O, 78);
    addRow("PRIMARY", O, 79);
    addRow("ATOM", idOnly, 80);
    addRow("ATOM", thisOnly, 81);
    addRow("ATOM", intLitOnly, 82);
    addRow("ATOM", charLitOnly, 83);
    addRow("ATOM", lparOnly, 84);
    addRow("ATOM", newOnly, 85);
    addRow("POST", dotOnly, 86);
    addRow("POST", U, 87);
    addRow("CALL_OPT", lparOnly, 88);
    addRow("CALL_OPT", Udot, 89);
    addRow("ARGS", E, 90);
    addRow("ARGS", rparOnly, 91);
    addRow("ARG_TAIL", commaOnly, 92);
    addRow("ARG_TAIL", rparOnly, 93);
}

static PNode* parseTokens(vector<Token>& tokens, string& error, int& errLine, int& errCol) {
    initGrammar();
    PNode* root = new PNode("PROGRAM");
    vector<pair<string, PNode*> > stack;
    stack.push_back(make_pair("$", (PNode*)NULL));
    stack.push_back(make_pair("PROGRAM", root));
    int pos = 0;
    int n = (int)tokens.size();
    while (!stack.empty()) {
        string X = stack.back().first;
        PNode* nx = stack.back().second;
        stack.pop_back();
        string a = (pos < n) ? tokens[pos].kind : "$";
        if (X == "$") {
            if (a == "$") return root;
            error = "unexpected token '" + tokens[pos].lexeme + "'";
            errLine = tokens[pos].line;
            errCol = tokens[pos].column;
            return NULL;
        }
        if (NONTERMS.find(X) == NONTERMS.end()) {
            if (a == X) {
                nx->lex = tokens[pos].lexeme;
                nx->line = tokens[pos].line;
                pos++;
                continue;
            }
            error = "unexpected token '" + tokens[pos].lexeme + "'";
            errLine = tokens[pos].line;
            errCol = tokens[pos].column;
            return NULL;
        }
        map<pair<string, string>, int>::iterator it = M.find(make_pair(X, a));
        if (it == M.end()) {
            error = "unexpected token '" + tokens[pos].lexeme + "'";
            errLine = tokens[pos].line;
            errCol = tokens[pos].column;
            return NULL;
        }
        vector<string>& rhs = RHS[it->second - 1];
        if (rhs.empty()) {
            PNode* eps = new PNode("EPS");
            eps->line = (pos < n) ? tokens[pos].line : (n > 0 ? tokens[n - 1].line : 0);
            nx->kids.push_back(eps);
            continue;
        }
        vector<PNode*> children;
        for (int j = 0; j < (int)rhs.size(); j++) {
            PNode* child = new PNode(rhs[j]);
            child->line = (pos < n) ? tokens[pos].line : (n > 0 ? tokens[n - 1].line : 0);
            children.push_back(child);
        }
        nx->kids = children;
        for (int j = (int)rhs.size() - 1; j >= 0; j--) {
            stack.push_back(make_pair(rhs[j], children[j]));
        }
    }
    return NULL;
}

static string convType(PNode* p) {
    PNode* first = p->kids[0];
    if (first->sym == "object") return "object:" + p->kids[1]->lex;
    return first->sym;
}

static Node* convBlock(PNode* p);
static Node* convEx(PNode* p);
static Node* convSt(PNode* p);

static vector<Node*> convSlist(PNode* p) {
    vector<Node*> out;
    while (p->kids[0]->sym != "EPS") {
        out.push_back(convSt(p->kids[0]));
        p = p->kids[1];
    }
    return out;
}

static Node* convTarget(PNode* p) {
    PNode* root = p->kids[0];
    Node* n;
    if (root->kids[0]->sym == "ID") n = new Node("Var", root->kids[0]->lex);
    else n = new Node("This");
    n->line = root->kids[0]->line;
    PNode* path = p->kids[1];
    while (path->kids[0]->sym != "EPS") {
        Node* fa = new Node("FieldAccess", path->kids[1]->lex);
        fa->line = path->kids[1]->line;
        fa->kids.push_back(n);
        n = fa;
        path = path->kids[2];
    }
    return n;
}

static vector<Node*> convParams(PNode* p) {
    vector<Node*> out;
    if (p->kids[0]->sym == "EPS") return out;
    Node* param = new Node("Param", p->kids[1]->lex);
    param->type = convType(p->kids[0]);
    param->line = p->kids[1]->line;
    out.push_back(param);
    PNode* tail = p->kids[2];
    while (tail->kids[0]->sym != "EPS") {
        Node* p2 = new Node("Param", tail->kids[2]->lex);
        p2->type = convType(tail->kids[1]);
        p2->line = tail->kids[2]->line;
        out.push_back(p2);
        tail = tail->kids[3];
    }
    return out;
}

static vector<Node*> convMembers(PNode* p) {
    vector<Node*> out;
    while (p->kids[0]->sym != "EPS") {
        PNode* m = p->kids[0];
        if (m->kids[0]->sym == "TYPE") {
            Node* f = new Node("Field", m->kids[1]->lex);
            f->type = convType(m->kids[0]);
            f->line = m->kids[1]->line;
            out.push_back(f);
        }
        else {
            Node* meth = new Node("Method", m->kids[2]->lex);
            meth->type = convType(m->kids[1]);
            meth->line = m->kids[2]->line;
            vector<Node*> params = convParams(m->kids[4]);
            for (int i = 0; i < (int)params.size(); i++) meth->kids.push_back(params[i]);
            meth->kids.push_back(convBlock(m->kids[6]));
            out.push_back(meth);
        }
        p = p->kids[1];
    }
    return out;
}

static Node* convClass(PNode* p) {
    Node* c = new Node("Class", p->kids[1]->lex);
    c->line = p->kids[1]->line;
    PNode* base = p->kids[2];
    if (base->kids[0]->sym != "EPS") c->type = base->kids[1]->lex;
    PNode* mem = p->kids[3]->kids[1];
    c->kids = convMembers(mem);
    return c;
}

static vector<Node*> convClasses(PNode* p) {
    vector<Node*> out;
    while (p->kids[0]->sym != "EPS") {
        out.push_back(convClass(p->kids[0]));
        p = p->kids[1];
    }
    return out;
}

static Node* convAtom(PNode* p) {
    PNode* a = p->kids[0];
    if (a->sym == "ID") {
        Node* n = new Node("Var", a->lex);
        n->line = a->line;
        return n;
    }
    if (a->sym == "this") {
        Node* n = new Node("This");
        n->line = a->line;
        return n;
    }
    if (a->sym == "INT_LIT") {
        Node* n = new Node("Int", a->lex);
        n->line = a->line;
        return n;
    }
    if (a->sym == "CHAR_LIT") {
        Node* n = new Node("Char", a->lex);
        n->line = a->line;
        return n;
    }
    if (a->sym == "(") return convEx(p->kids[1]);
    Node* n = new Node("New", p->kids[1]->lex);
    n->line = a->line;
    return n;
}

static Node* convPrimary(PNode* p) {
    Node* n = convAtom(p->kids[0]);
    PNode* post = p->kids[1];
    while (post->kids[0]->sym != "EPS") {
        PNode* callOpt = post->kids[2];
        if (callOpt->kids[0]->sym != "EPS") {
            Node* c = new Node("Call", post->kids[1]->lex);
            c->line = post->kids[1]->line;
            c->kids.push_back(n);
            PNode* args = callOpt->kids[1];
            if (args->kids[0]->sym != "EPS") {
                c->kids.push_back(convEx(args->kids[0]));
                PNode* tail = args->kids[1];
                while (tail->kids[0]->sym != "EPS") {
                    c->kids.push_back(convEx(tail->kids[1]));
                    tail = tail->kids[2];
                }
            }
            n = c;
        }
        else {
            Node* fa = new Node("FieldAccess", post->kids[1]->lex);
            fa->line = post->kids[1]->line;
            fa->kids.push_back(n);
            n = fa;
        }
        post = post->kids[3];
    }
    return n;
}

static Node* convUnary(PNode* p) {
    if (p->kids[0]->sym == "-") {
        Node* n = new Node("Unary", "-");
        n->line = p->kids[0]->line;
        n->kids.push_back(convUnary(p->kids[1]));
        return n;
    }
    if (p->kids[0]->sym == "not") {
        Node* n = new Node("Unary", "not");
        n->line = p->kids[0]->line;
        n->kids.push_back(convUnary(p->kids[1]));
        return n;
    }
    return convPrimary(p->kids[0]);
}

static Node* convTerm(PNode* p) {
    Node* n = convUnary(p->kids[0]);
    PNode* tail = p->kids[1];
    while (tail->kids[0]->sym != "EPS") {
        Node* b = new Node("Binary", tail->kids[0]->sym);
        b->kids.push_back(n);
        b->kids.push_back(convUnary(tail->kids[1]));
        n = b;
        tail = tail->kids[2];
    }
    return n;
}

static Node* convAdd(PNode* p) {
    Node* n = convTerm(p->kids[0]);
    PNode* tail = p->kids[1];
    while (tail->kids[0]->sym != "EPS") {
        Node* b = new Node("Binary", tail->kids[0]->sym);
        b->kids.push_back(n);
        b->kids.push_back(convTerm(tail->kids[1]));
        n = b;
        tail = tail->kids[2];
    }
    return n;
}

static Node* convRelExpr(PNode* p) {
    Node* n = convAdd(p->kids[0]);
    PNode* tail = p->kids[1];
    if (tail->kids[0]->sym != "EPS") {
        Node* b = new Node("Binary", tail->kids[0]->kids[0]->sym);
        b->kids.push_back(n);
        b->kids.push_back(convAdd(tail->kids[1]));
        n = b;
    }
    return n;
}

static Node* convAndExpr(PNode* p) {
    Node* n = convRelExpr(p->kids[0]);
    PNode* tail = p->kids[1];
    while (tail->kids[0]->sym != "EPS") {
        Node* b = new Node("Binary", "and");
        b->kids.push_back(n);
        b->kids.push_back(convRelExpr(tail->kids[1]));
        n = b;
        tail = tail->kids[2];
    }
    return n;
}

static Node* convOrExpr(PNode* p) {
    Node* n = convAndExpr(p->kids[0]);
    PNode* tail = p->kids[1];
    while (tail->kids[0]->sym != "EPS") {
        Node* b = new Node("Binary", "or");
        b->kids.push_back(n);
        b->kids.push_back(convAndExpr(tail->kids[1]));
        n = b;
        tail = tail->kids[2];
    }
    return n;
}

static Node* convEx(PNode* p) {
    return convOrExpr(p->kids[0]);
}

static Node* convSt(PNode* p) {
    PNode* s = p->kids[0];
    if (s->sym == "DS") {
        Node* d = new Node("Decl", s->kids[1]->lex);
        d->type = convType(s->kids[0]);
        d->line = s->kids[1]->line;
        return d;
    }
    if (s->sym == "AS") {
        Node* a = new Node("Assign");
        a->line = s->kids[0]->kids[0]->kids[0]->line;
        a->kids.push_back(convTarget(s->kids[0]));
        a->kids.push_back(convEx(s->kids[2]));
        return a;
    }
    if (s->sym == "WS") {
        Node* w = new Node("While");
        w->line = s->kids[0]->line;
        w->kids.push_back(convEx(s->kids[2]));
        w->kids.push_back(convBlock(s->kids[4]));
        return w;
    }
    if (s->sym == "FS") {
        Node* f = new Node("For");
        f->line = s->kids[0]->line;
        Node* init = new Node("Assign");
        init->kids.push_back(convTarget(s->kids[2]->kids[0]));
        init->kids.push_back(convEx(s->kids[2]->kids[2]));
        Node* update = new Node("Assign");
        update->kids.push_back(convTarget(s->kids[6]->kids[0]));
        update->kids.push_back(convEx(s->kids[6]->kids[2]));
        f->kids.push_back(init);
        f->kids.push_back(convEx(s->kids[4]));
        f->kids.push_back(update);
        f->kids.push_back(convBlock(s->kids[8]));
        return f;
    }
    if (s->sym == "IS") {
        Node* ifn = new Node("If");
        ifn->line = s->kids[0]->line;
        Node* br = new Node("Branch");
        br->kids.push_back(convEx(s->kids[2]));
        br->kids.push_back(convBlock(s->kids[4]));
        ifn->kids.push_back(br);
        PNode* it = s->kids[5];
        Node* elseB = NULL;
        while (it->kids[0]->sym != "EPS") {
            if (it->kids[0]->sym == "wrna") {
                Node* b2 = new Node("Branch");
                b2->kids.push_back(convEx(it->kids[2]));
                b2->kids.push_back(convBlock(it->kids[4]));
                ifn->kids.push_back(b2);
                it = it->kids[5];
            }
            else {
                elseB = convBlock(it->kids[1]);
                break;
            }
        }
        if (elseB != NULL) ifn->kids.push_back(elseB);
        return ifn;
    }
    if (s->sym == "CST") return convBlock(s);
    if (s->sym == "RS") {
        Node* r = new Node("Return");
        r->line = s->kids[0]->line;
        r->kids.push_back(convEx(s->kids[1]));
        return r;
    }
    if (s->sym == "BS") {
        Node* b = new Node("Break");
        b->line = s->kids[0]->line;
        return b;
    }
    if (s->sym == "CS") {
        Node* c = new Node("Continue");
        c->line = s->kids[0]->line;
        return c;
    }
    Node* pr = new Node("Print");
    pr->line = s->kids[0]->line;
    pr->kids.push_back(convEx(s->kids[1]));
    return pr;
}

static Node* convBlock(PNode* p) {
    Node* b = new Node("Block");
    b->kids = convSlist(p->kids[1]);
    return b;
}

static Node* buildAst(PNode* root) {
    Node* prog = new Node("Program");
    vector<Node*> classes = convClasses(root->kids[0]);
    vector<Node*> stmts = convSlist(root->kids[1]);
    for (int i = 0; i < (int)classes.size(); i++) prog->kids.push_back(classes[i]);
    for (int i = 0; i < (int)stmts.size(); i++) prog->kids.push_back(stmts[i]);
    return prog;
}

static void printAst(Node* n, int depth) {
    for (int i = 0; i < depth; i++) cout << "  ";
    cout << n->kind;
    if (n->kind == "Field" || n->kind == "Decl" || n->kind == "Param" || n->kind == "Method") {
        cout << " " << n->text << " : " << n->type;
    }
    else if (n->kind == "Class") {
        cout << " " << n->text;
        if (n->type != "") cout << " extends " << n->type;
    }
    else if (n->kind == "FieldAccess" || n->kind == "Call" || n->kind == "New") {
        cout << " " << n->text;
    }
    else if (n->text != "") {
        cout << " " << n->text;
    }
    cout << endl;
    for (int i = 0; i < (int)n->kids.size(); i++) {
        printAst(n->kids[i], depth + 1);
    }
}

static void freeAst(Node* n) {
    for (int i = 0; i < (int)n->kids.size(); i++) freeAst(n->kids[i]);
    delete n;
}

#endif
