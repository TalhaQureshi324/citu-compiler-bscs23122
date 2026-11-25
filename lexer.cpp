#include "lexer.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "usage: lexer <program.citu>" << endl;
        return 1;
    }
    ifstream input(argv[1]);
    if (!input) {
        cout << "Could not open input file: " << argv[1] << endl;
        return 1;
    }
    string code((istreambuf_iterator<char>(input)), istreambuf_iterator<char>());
    string error;
    vector<Token> tokens = tokenize(code, error);
    cout << "#  TOKEN      LEXEME                LINE  COLUMN" << endl;
    for (int i = 0; i < (int)tokens.size(); i++) {
        Token t = tokens[i];
        cout << t.index << "  " << t.kind << "  " << t.lexeme << "  " << t.line << "  " << t.column << endl;
    }
    if (error != "") {
        cout << error << endl;
        cout << "Lexical analysis failed." << endl;
        return 1;
    }
    cout << "Total tokens: " << tokens.size() << " (including EOF)" << endl;
    cout << "Lexical analysis passed." << endl;
    return 0;
}
