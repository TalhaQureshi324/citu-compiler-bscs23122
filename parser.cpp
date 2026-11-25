#include "parser.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "usage: parser <program.citu>" << endl;
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
    cout << "Parsing accepted (" << tokens.size() << " tokens including EOF)." << endl;
    cout << endl;
    cout << "AST:" << endl;
    printAst(ast, 0);
    return 0;
}
