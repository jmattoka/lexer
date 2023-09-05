/*
 * Copyright (C) Rida Bazzi, 2016
 *
 * Do not share this file with anyone
 */

#include <iostream>
#include <istream>
#include <vector>
#include <string>
#include <cctype>

#include "lexer.h"
#include "inputbuf.h"

#include "inputbuf.cc"
using namespace std;

string reserved[] = { "END_OF_FILE",
    "IF", "WHILE", "DO", "THEN", "PRINT",
    "PLUS", "MINUS", "DIV", "MULT",
    "EQUAL", "COLON", "COMMA", "SEMICOLON",
    "LBRAC", "RBRAC", "LPAREN", "RPAREN",
    "NOTEQUAL", "GREATER", "LESS", "LTEQ", "GTEQ",
    "DOT", "NUM", "ID", "ERROR", 
    "REALNUM", "BASE08NUM", "BASE16NUM" // TODO: Add labels for new token types here (as string)
};

#define KEYWORDS_COUNT 5
string keyword[] = { "IF", "WHILE", "DO", "THEN", "PRINT" };

void Token::Print()
{
    cout << "{" << this->lexeme << " , "
         << reserved[(int) this->token_type] << " , "
         << this->line_no << "}\n";
}

LexicalAnalyzer::LexicalAnalyzer()
{
    this->line_no = 1;
    tmp.lexeme = "";
    tmp.line_no = 1;
    tmp.token_type = ERROR;
}

bool LexicalAnalyzer::SkipSpace()
{
    char c;
    bool space_encountered = false;

    input.GetChar(c);
    line_no += (c == '\n');

    while (!input.EndOfInput() && isspace(c)) {
        space_encountered = true;
        input.GetChar(c);
        line_no += (c == '\n');
    }

    if (!input.EndOfInput()) {
        input.UngetChar(c);
    }
    return space_encountered;
}

bool LexicalAnalyzer::IsKeyword(string s)
{
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (s == keyword[i]) {
            return true;
        }
    }
    return false;
}

TokenType LexicalAnalyzer::FindKeywordIndex(string s)
{
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (s == keyword[i]) {
            return (TokenType) (i + 1);
        }
    }
    return ERROR;
}

Token LexicalAnalyzer::ScanNumber()
{
    char c;
    string s1;      //used to combine all chars grabbed from input
    tmp.token_type = ERROR;
    input.GetChar(c);
    if (isdigit(c)) {
        if (c == '0') {
            tmp.lexeme = "0";
        } else {
            tmp.lexeme = "";
            while (!input.EndOfInput() && isdigit(c)) {
                tmp.lexeme += c;
                input.GetChar(c);
            }
            if (!input.EndOfInput()) {
                input.UngetChar(c);
            }
        }
        // TODO: You can check for REALNUM, BASE08NUM and BASE16NUM here!
        //Check for BASE08NUM
        bool hasBase08Potential = false;
        for(int i = 0; i < tmp.lexeme.size() && !input.EndOfInput(); i++){  //check if all numbers are valid base08 nums
            if(tmp.lexeme[i] - 48 > 7){
                hasBase08Potential = false;
                break;
            }
            else
                hasBase08Potential = true;
        }
        if(hasBase08Potential){ //if it has potential continue, otherwise return everything back 
            s1 = "";
            for(int i = 0; i < 3 && !input.EndOfInput(); i++){  //get the next 3 characters, check for x08
                input.GetChar(c);
                s1 += c;
            }
            if(s1 == "x08"){    //if it is x08, then we have a base08 number
                tmp.lexeme += s1;
                tmp.token_type = BASE08NUM;
                tmp.line_no = line_no;
                return tmp;
            }
            else{   //otherwise, return everything back 
                input.UngetString(s1);
                s1.clear();
            }
        }
        //call helper function to check for base 16
        ScanBase16();
        if(tmp.token_type == BASE16NUM){    //if base 16, then return it
            return tmp;
        }
        //check for REALNUM
        for(int i = 0; i < 2 && !input.EndOfInput(); i++){  
            input.GetChar(c);
            s1 += c;
        }
        if(s1[0] == '.' && isdigit(s1[1])){ //check if .[number] exists; if so, grab all numbers after
            tmp.lexeme += s1;
            input.GetChar(c);
            while (!input.EndOfInput() && isdigit(c)){
                tmp.lexeme += c;
                input.GetChar(c);
            }
            if (!input.EndOfInput()) {
                input.UngetChar(c);
            }
            tmp.token_type = REALNUM;
            tmp.line_no = line_no;
            return tmp;
        }
        else{   //otherwise, return everything back 
            input.UngetString(s1);
            s1.clear();
        }
        tmp.token_type = NUM;   //base case, it is a plain number, return it 
        tmp.line_no = line_no;
        return tmp;
    } 
    else {  //error
        if (!input.EndOfInput()) {
            input.UngetChar(c);
        }
        tmp.lexeme = "";
        tmp.token_type = ERROR;
        tmp.line_no = line_no;
        return tmp;
    }
    return tmp;
}

Token LexicalAnalyzer::ScanIdOrKeyword()
{
    char c;
    // ScanBase16();   //call helper function to detect base 16 for base16 values starting with A,B,C,D,E,F
    // if(tmp.token_type == BASE16NUM) //if base 16, return 
    //     return tmp;
    input.GetChar(c);
    if (isalpha(c)) {
        tmp.lexeme = "";
        while (!input.EndOfInput() && isalnum(c)) { //grab all valid characters
            tmp.lexeme += c;
            input.GetChar(c);
        }
        if (!input.EndOfInput()) {
            input.UngetChar(c);
        }
        tmp.line_no = line_no;
        if (IsKeyword(tmp.lexeme))  //check for keyword
            tmp.token_type = FindKeywordIndex(tmp.lexeme);
        else
            tmp.token_type = ID;    //otherwise, it is an ID
    } else {
        if (!input.EndOfInput()) {
            input.UngetChar(c);
        }
        tmp.lexeme = "";    //ERROR
        tmp.token_type = ERROR;
    }
    return tmp;
}

Token LexicalAnalyzer::ScanBase16(){
    char c;
    string s1;

    input.GetChar(c);   
    while((isdigit(c) || (c >= 65 && c <= 70)) && !input.EndOfInput()){ //grab all valid base 16 characters
        s1 += c;
        input.GetChar(c);
    }
    if (!input.EndOfInput()) {
        input.UngetChar(c);
    }
    string s2 = "";
    for(int i = 0; i < 3 && !input.EndOfInput(); i++){  //grab next 3 characters, check for "x16"
        input.GetChar(c);
        s2 += c;
    }
    //edge cases; handles 0x16, x16, and checks for valid base 16, etc.
    if(s2 == "x16" && tmp.lexeme.size() != 0 && (tmp.lexeme[0] != '0' || (tmp.lexeme[0] + s1 + s2) == "0x16")){
        tmp.lexeme += s1 + s2;
        tmp.token_type = BASE16NUM;
        tmp.line_no = line_no;
        return tmp;
    }
    else{   //if not valid base16 value, return everything as was
        input.UngetString(s2);
        input.UngetString(s1);
        s1.clear();
        s2.clear();
    }
    return tmp;
}

// you should unget tokens in the reverse order in which they
// are obtained. If you execute
//
//    t1 = lexer.GetToken();
//    t2 = lexer.GetToken();
//    t3 = lexer.GetToken();
//
// in this order, you should execute
//
//    lexer.UngetToken(t3);
//    lexer.UngetToken(t2);
//    lexer.UngetToken(t1);
//
// if you want to unget all three tokens. Note that it does not
// make sense to unget t1 without first ungetting t2 and t3
//
TokenType LexicalAnalyzer::UngetToken(Token tok)
{
    tokens.push_back(tok);;
    return tok.token_type;
}

Token LexicalAnalyzer::GetToken()
{
    char c;

    // if there are tokens that were previously
    // stored due to UngetToken(), pop a token and
    // return it without reading from input
    if (!tokens.empty()) {
        tmp = tokens.back();
        tokens.pop_back();
        return tmp;
    }

    SkipSpace();
    tmp.lexeme = "";
    tmp.line_no = line_no;
    input.GetChar(c);
    switch (c) {
        case '.':
            tmp.token_type = DOT;
            return tmp;
        case '+':
            tmp.token_type = PLUS;
            return tmp;
        case '-':
            tmp.token_type = MINUS;
            return tmp;
        case '/':
            tmp.token_type = DIV;
            return tmp;
        case '*':
            tmp.token_type = MULT;
            return tmp;
        case '=':
            tmp.token_type = EQUAL;
            return tmp;
        case ':':
            tmp.token_type = COLON;
            return tmp;
        case ',':
            tmp.token_type = COMMA;
            return tmp;
        case ';':
            tmp.token_type = SEMICOLON;
            return tmp;
        case '[':
            tmp.token_type = LBRAC;
            return tmp;
        case ']':
            tmp.token_type = RBRAC;
            return tmp;
        case '(':
            tmp.token_type = LPAREN;
            return tmp;
        case ')':
            tmp.token_type = RPAREN;
            return tmp;
        case '<':
            input.GetChar(c);
            if (c == '=') {
                tmp.token_type = LTEQ;
            } else if (c == '>') {
                tmp.token_type = NOTEQUAL;
            } else {
                if (!input.EndOfInput()) {
                    input.UngetChar(c);
                }
                tmp.token_type = LESS;
            }
            return tmp;
        case '>':
            input.GetChar(c);
            if (c == '=') {
                tmp.token_type = GTEQ;
            } else {
                if (!input.EndOfInput()) {
                    input.UngetChar(c);
                }
                tmp.token_type = GREATER;
            }
            return tmp;
        default:
            if (isdigit(c)) {   
                input.UngetChar(c);
                return ScanNumber();
            } else if (isalpha(c)) {
                input.UngetChar(c);
                return ScanIdOrKeyword();
            } else if (input.EndOfInput())
                tmp.token_type = END_OF_FILE;
            else
                tmp.token_type = ERROR;

            return tmp;
    }
}

int main()
{
    LexicalAnalyzer lexer;
    Token token;

    token = lexer.GetToken();
    token.Print();
    while (token.token_type != END_OF_FILE)
    {
        token = lexer.GetToken();
        token.Print();
    }
}
