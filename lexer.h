#ifndef LEXER_H
#define LEXER_H
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

struct Token {
    int index;
    string kind;
    string lexeme;
    int line;
    int column;
};

bool isLetter(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
bool isDigit(char c) { return c >= '0' && c <= '9'; }

bool isCituKeyword(const string& word) {
    static const string keywords[] = {
        "int16", "int32", "uint16", "uint32", "char",
        "jabtak", "for", "agr", "wrna", "wrna_phir", "break", "continue",
        "shuru", "khtm", "medalo",
        "class", "extends", "object", "method", "new", "this", "return",
        "and", "or", "not", "print"
    };
    int n = sizeof(keywords) / sizeof(keywords[0]);
    for (int i = 0; i < n; i++) {
        if (word == keywords[i]) return true;
    }
    return false;
}

bool isValidEscape(char c) {
    return c == 'n' || c == 't' || c == 'r' || c == '0' || c == '\\' || c == '\'';
}

bool isPrintableAscii(char c) {
    int v = (unsigned char)c;
    return v >= 32 && v <= 126;
}

vector<Token> tokenize(const string& code, string& error) {
    vector<Token> tokens;
    int i = 0, line = 1, col = 1, index = 1;
    int len = (int)code.length();
    error = "";
    while (i < len) {
        char c = code[i];
        if (c == ' ' || c == '\t') {
            i++;
            col++;
            continue;
        }
        if (c == '\r') {
            i++;
            if (i < len && code[i] == '\n') i++;
            line++;
            col = 1;
            continue;
        }
        if (c == '\n') {
            i++;
            line++;
            col = 1;
            continue;
        }
        if (c == '/' && i + 1 < len && code[i + 1] == '/') {
            while (i < len && code[i] != '\n') {
                i++;
                col++;
            }
            continue;
        }
        int sl = line, sc = col;
        if (isLetter(c) || c == '_') {
            string word = "";
            while (i < len && (isLetter(code[i]) || isDigit(code[i]) || code[i] == '_')) {
                word += code[i];
                i++;
                col++;
            }
            string kind = isCituKeyword(word) ? word : "ID";
            tokens.push_back({index++, kind, word, sl, sc});
            continue;
        }
        if (isDigit(c)) {
            string num = "";
            while (i < len && isDigit(code[i])) {
                num += code[i];
                i++;
                col++;
            }
            if (i < len && (isLetter(code[i]) || code[i] == '_')) {
                error = "Lexical error at line " + to_string(sl) + ", column " + to_string(sc) + ": malformed numeric token '" + num + code[i] + "'";
                return tokens;
            }
            tokens.push_back({index++, "INT_LIT", num, sl, sc});
            continue;
        }
        if (c == '\'') {
            string lit = "'";
            i++;
            col++;
            if (i < len && code[i] == '\\') {
                lit += code[i];
                i++;
                col++;
                if (i >= len || code[i] == '\n') {
                    error = "Lexical error at line " + to_string(sl) + ", column " + to_string(sc) + ": unterminated character literal";
                    return tokens;
                }
                if (!isValidEscape(code[i])) {
                    error = "Lexical error at line " + to_string(sl) + ", column " + to_string(sc) + ": invalid character escape '\\" + string(1, code[i]) + "'";
                    return tokens;
                }
                lit += code[i];
                i++;
                col++;
            }
            else if (i < len && code[i] != '\'' && code[i] != '\n' && isPrintableAscii(code[i])) {
                lit += code[i];
                i++;
                col++;
            }
            else {
                error = "Lexical error at line " + to_string(sl) + ", column " + to_string(sc) + ": invalid character literal";
                return tokens;
            }
            if (i < len && code[i] == '\'') {
                lit += '\'';
                i++;
                col++;
            }
            else {
                error = "Lexical error at line " + to_string(sl) + ", column " + to_string(sc) + ": unterminated character literal";
                return tokens;
            }
            tokens.push_back({index++, "CHAR_LIT", lit, sl, sc});
            continue;
        }
        if (c == '=') {
            if (i + 1 < len && code[i + 1] == '=') {
                error = "Lexical error at line " + to_string(sl) + ", column " + to_string(sc) + ": invalid operator '==', use single '=' for comparison";
                return tokens;
            }
            tokens.push_back({index++, "=", "=", sl, sc});
            i++;
            col++;
            continue;
        }
        if (c == '+') {
            if (i + 1 < len && code[i + 1] == '+') {
                error = "Lexical error at line " + to_string(sl) + ", column " + to_string(sc) + ": invalid operator '++'";
                return tokens;
            }
            tokens.push_back({index++, "+", "+", sl, sc});
            i++;
            col++;
            continue;
        }
        if (c == '-') {
            if (i + 1 < len && code[i + 1] == '-') {
                error = "Lexical error at line " + to_string(sl) + ", column " + to_string(sc) + ": invalid operator '--'";
                return tokens;
            }
            tokens.push_back({index++, "-", "-", sl, sc});
            i++;
            col++;
            continue;
        }
        if (c == '!') {
            if (i + 1 < len && code[i + 1] == '=') {
                tokens.push_back({index++, "!=", "!=", sl, sc});
                i += 2;
                col += 2;
                continue;
            }
            error = "Lexical error at line " + to_string(sl) + ", column " + to_string(sc) + ": invalid character '!'";
            return tokens;
        }
        if (c == '<' || c == '>') {
            string op(1, c);
            if (i + 1 < len && code[i + 1] == '=') {
                op += '=';
                i++;
                col++;
            }
            tokens.push_back({index++, op, op, sl, sc});
            i++;
            col++;
            continue;
        }
        if (c == '*' || c == '/') {
            string op(1, c);
            tokens.push_back({index++, op, op, sl, sc});
            i++;
            col++;
            continue;
        }
        if (c == '(' || c == ')' || c == '.' || c == ',' || c == ';') {
            string op(1, c);
            tokens.push_back({index++, op, op, sl, sc});
            i++;
            col++;
            continue;
        }
        error = "Lexical error at line " + to_string(sl) + ", column " + to_string(sc) + ": invalid character '" + string(1, c) + "'";
        return tokens;
    }
    tokens.push_back({index++, "EOF", "", line, col});
    return tokens;
}

#endif
