/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2012 Daniel Marjamäki and Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "testsuite.h"
#include "testutils.h"
#include "token.h"
#include "settings.h"

#include <vector>
#include <string>

extern std::ostringstream errout;
class TestToken : public TestFixture {
public:
    TestToken() : TestFixture("TestToken")
    { }

private:
    std::vector<std::string> arithmeticalOps;
    std::vector<std::string> logicalOps;
    std::vector<std::string> bitOps;
    std::vector<std::string> comparisionOps;
    std::vector<std::string> extendedOps;
    std::vector<std::string> assignmentOps;

    void run() {
        initOps();

        TEST_CASE(nextprevious);
        TEST_CASE(multiCompare);
        TEST_CASE(multiCompare2);                   // #3294 - false negative multi compare between "=" and "=="
        TEST_CASE(multiCompare3);                   // false positive for %or% on code using "|="
        TEST_CASE(getStrLength);
        TEST_CASE(strValue);

        TEST_CASE(deleteLast);
        TEST_CASE(nextArgument);
        TEST_CASE(eraseTokens);

        TEST_CASE(matchAny);
        TEST_CASE(matchSingleChar);
        TEST_CASE(matchNothingOrAnyNotElse);
        TEST_CASE(matchType);
        TEST_CASE(matchChar);
        TEST_CASE(matchCompOp);
        TEST_CASE(matchStr);
        TEST_CASE(matchVarid);
        TEST_CASE(matchNumeric);
        TEST_CASE(matchBoolean);
        TEST_CASE(matchOr);
        TEST_CASE(matchOp);

        TEST_CASE(isArithmeticalOp);
        TEST_CASE(isOp);
        TEST_CASE(isExtendedOp);
        TEST_CASE(isAssignmentOp);
        TEST_CASE(isStandardType);
        TEST_CASE(literals);
        TEST_CASE(operators);

        TEST_CASE(updateProperties)
        TEST_CASE(updatePropertiesConcatStr)
        TEST_CASE(isNameGuarantees1)
        TEST_CASE(isNameGuarantees2)
        TEST_CASE(isNameGuarantees3)
        TEST_CASE(isNameGuarantees4)
        TEST_CASE(isNameGuarantees5)

        TEST_CASE(canFindMatchingBracketsNeedsOpen);
        TEST_CASE(canFindMatchingBracketsInnerPair);
        TEST_CASE(canFindMatchingBracketsOuterPair);
        TEST_CASE(canFindMatchingBracketsWithTooManyClosing);
        TEST_CASE(canFindMatchingBracketsWithTooManyOpening);
    }

    void nextprevious() {
        Token *token = new Token(0);
        token->str("1");
        token->insertToken("2");
        token->next()->insertToken("3");
        Token *last = token->tokAt(2);
        ASSERT_EQUALS(token->str(), "1");
        ASSERT_EQUALS(token->next()->str(), "2");
        ASSERT_EQUALS(token->tokAt(2)->str(), "3");
        if (last->next())
            ASSERT_EQUALS("Null was expected", "");

        ASSERT_EQUALS(last->str(), "3");
        ASSERT_EQUALS(last->previous()->str(), "2");
        ASSERT_EQUALS(last->tokAt(-2)->str(), "1");
        if (token->previous())
            ASSERT_EQUALS("Null was expected", "");

        TokenList::deleteTokens(token);
    }

    bool Match(const std::string &code, const std::string &pattern, unsigned int varid=0) {
        static const Settings settings;
        Tokenizer tokenizer(&settings, this);
        std::istringstream istr(code);
        tokenizer.tokenize(istr, "test.cpp");
        return Token::Match(tokenizer.tokens(), pattern.c_str(), varid);
    }

    void multiCompare() {
        // Test for found
        ASSERT_EQUALS(1, Token::multiCompare("one|two", "one"));
        ASSERT_EQUALS(1, Token::multiCompare("one|two", "two"));
        ASSERT_EQUALS(1, Token::multiCompare("verybig|two|", "two"));

        // Test for empty string found
        ASSERT_EQUALS(0, Token::multiCompare("|one|two", "notfound"));
        ASSERT_EQUALS(0, Token::multiCompare("one||two", "notfound"));
        ASSERT_EQUALS(0, Token::multiCompare("one|two|", "notfound"));

        // Test for not found
        ASSERT_EQUALS(static_cast<unsigned int>(-1), static_cast<unsigned int>(Token::multiCompare("one|two", "notfound")));
        ASSERT_EQUALS(static_cast<unsigned int>(-1), static_cast<unsigned int>(Token::multiCompare("verybig|two", "s")));
        ASSERT_EQUALS(static_cast<unsigned int>(-1), static_cast<unsigned int>(Token::multiCompare("one|two", "ne")));
        ASSERT_EQUALS(static_cast<unsigned int>(-1), static_cast<unsigned int>(Token::multiCompare("abc|def", "a")));
        ASSERT_EQUALS(static_cast<unsigned int>(-1), static_cast<unsigned int>(Token::multiCompare("abc|def", "abcd")));
        ASSERT_EQUALS(static_cast<unsigned int>(-1), static_cast<unsigned int>(Token::multiCompare("abc|def", "default")));

        // %op%
        ASSERT_EQUALS(1, Token::multiCompare("one|%op%", "+"));
        ASSERT_EQUALS(1, Token::multiCompare("%op%|two", "+"));
        ASSERT_EQUALS(-1, Token::multiCompare("one|%op%", "x"));
        ASSERT_EQUALS(-1, Token::multiCompare("%op%|two", "x"));
    }

    void multiCompare2() { // #3294
        // Original pattern that failed: [[,(=<>+-*|&^] %num% [+-*/] %num% ]|,|)|;|=|%op%
        givenACodeSampleToTokenize toks("a == 1", true);
        ASSERT_EQUALS(true, Token::Match(toks.tokens(), "a =|%op%"));
    }

    void multiCompare3() {
        // Original pattern that failed: "return|(|&&|%oror% %var% &&|%oror%|==|!=|<=|>=|<|>|-|%or% %var% )|&&|%oror%|;"
        // Code snippet that failed: "return lv@86 |= rv@87 ;"

        // Note: Also test "reverse" alternative pattern, two different code paths to handle it
        givenACodeSampleToTokenize toks("return a |= b ;", true);
        ASSERT_EQUALS(false, Token::Match(toks.tokens(), "return %var% xyz|%or% %var% ;"));
        ASSERT_EQUALS(false, Token::Match(toks.tokens(), "return %var% %or%|xyz %var% ;"));

        givenACodeSampleToTokenize toks2("return a | b ;", true);
        ASSERT_EQUALS(true, Token::Match(toks2.tokens(), "return %var% xyz|%or% %var% ;"));
        ASSERT_EQUALS(true, Token::Match(toks2.tokens(), "return %var% %or%|xyz %var% ;"));

        givenACodeSampleToTokenize toks3("return a || b ;", true);
        ASSERT_EQUALS(false, Token::Match(toks3.tokens(), "return %var% xyz|%or% %var% ;"));
        ASSERT_EQUALS(false, Token::Match(toks3.tokens(), "return %var% %or%|xyz %var% ;"));

        ASSERT_EQUALS(true, Token::Match(toks3.tokens(),  "return %var% xyz|%oror% %var% ;"));
        ASSERT_EQUALS(true, Token::Match(toks3.tokens(),  "return %var% %oror%|xyz %var% ;"));

        givenACodeSampleToTokenize toks4("a % b ;", true);
        ASSERT_EQUALS(true, Token::Match(toks4.tokens(), "%var% >>|<<|&|%or%|^|% %var% ;"));
        ASSERT_EQUALS(true, Token::Match(toks4.tokens(), "%var% %|>>|<<|&|%or%|^ %var% ;"));
        ASSERT_EQUALS(true, Token::Match(toks4.tokens(), "%var% >>|<<|&|%or%|%|^ %var% ;"));

        //%var%|%num% support
        givenACodeSampleToTokenize num("100", true);
        ASSERT_EQUALS(true, Token::Match(num.tokens(), "%num%|%var%"));
        ASSERT_EQUALS(true, Token::Match(num.tokens(), "%var%|%num%"));
        ASSERT_EQUALS(true, Token::Match(num.tokens(), "%var%|%num%|%bool%"));
        ASSERT_EQUALS(true, Token::Match(num.tokens(), "%var%|%bool%|%num%"));
        ASSERT_EQUALS(true, Token::Match(num.tokens(), "%var%|%bool%|%str%|%num%"));
        ASSERT_EQUALS(false, Token::Match(num.tokens(), "%bool%|%var%"));
        ASSERT_EQUALS(false, Token::Match(num.tokens(), "%type%|%bool%|%char%"));
        ASSERT_EQUALS(true, Token::Match(num.tokens(), "%type%|%bool%|100"));

        givenACodeSampleToTokenize numparen("( 100 )", true);
        ASSERT_EQUALS(true, Token::Match(numparen.tokens(), "(| %num%|%var% )|"));
        ASSERT_EQUALS(true, Token::Match(numparen.tokens(), "(| %var%|%num% )|"));
        ASSERT_EQUALS(true, Token::Match(numparen.tokens(), "(| %var%|%num%|%bool% )|"));
        ASSERT_EQUALS(true, Token::Match(numparen.tokens(), "(| %var%|%bool%|%num% )|"));
        ASSERT_EQUALS(true, Token::Match(numparen.tokens(), "(| %var%|%bool%|%str%|%num% )|"));
        ASSERT_EQUALS(false, Token::Match(numparen.tokens(), "(| %bool%|%var% )|"));

        ASSERT_EQUALS(true, Token::Match(numparen.tokens(), "(| 100 %num%|%var%| )|"));
        ASSERT_EQUALS(true, Token::Match(numparen.tokens(), "(| 100 %var%|%num%| )|"));
        ASSERT_EQUALS(true, Token::Match(numparen.tokens(), "(| 100 %var%|%num%|%bool%| )|"));
        ASSERT_EQUALS(true, Token::Match(numparen.tokens(), "(| 100 %var%|%bool%|%num%| )|"));
        ASSERT_EQUALS(true, Token::Match(numparen.tokens(), "(| 100 %var%|%bool%|%str%|%num%| )|"));
        ASSERT_EQUALS(true, Token::Match(numparen.tokens(), "(| 100 %bool%|%var%| )|"));
    }

    void getStrLength() {
        Token tok(0);

        tok.str("\"\"");
        ASSERT_EQUALS(0, (int)Token::getStrLength(&tok));

        tok.str("\"test\"");
        ASSERT_EQUALS(4, (int)Token::getStrLength(&tok));

        tok.str("\"test \\\\test\"");
        ASSERT_EQUALS(10, (int)Token::getStrLength(&tok));

        tok.str("\"a\\0\"");
        ASSERT_EQUALS(1, (int)Token::getStrLength(&tok));
    }

    void strValue() {
        Token tok(0);
        tok.str("\"\"");
        ASSERT_EQUALS(std::string(""), tok.strValue());

        tok.str("\"0\"");
        ASSERT_EQUALS(std::string("0"), tok.strValue());
    }


    void deleteLast() {
        Token *tokensBack = 0;
        Token tok(&tokensBack);
        tok.insertToken("aba");
        ASSERT_EQUALS(true, tokensBack == tok.next());
        tok.deleteNext();
        ASSERT_EQUALS(true, tokensBack == &tok);
    }

    void nextArgument() {
        givenACodeSampleToTokenize example1("foo(1, 2, 3, 4);");
        ASSERT_EQUALS(true, Token::simpleMatch(example1.tokens()->tokAt(2)->nextArgument(), "2 , 3"));

        givenACodeSampleToTokenize example2("foo();");
        ASSERT_EQUALS(true, example2.tokens()->tokAt(2)->nextArgument() == 0);

        givenACodeSampleToTokenize example3("foo(bar(a, b), 2, 3);");
        ASSERT_EQUALS(true, Token::simpleMatch(example3.tokens()->tokAt(2)->nextArgument(), "2 , 3"));
    }

    void eraseTokens() {
        givenACodeSampleToTokenize code("begin ; { this code will be removed } end", true);
        Token::eraseTokens(code.tokens()->next(), code.tokens()->tokAt(9));
        ASSERT_EQUALS("begin ; end", code.tokens()->stringifyList(0, false));
    }


    void matchAny() {
        givenACodeSampleToTokenize varBitOrVar("abc|def", true);
        ASSERT_EQUALS(true, Token::Match(varBitOrVar.tokens(), "%var% | %var%"));

        givenACodeSampleToTokenize varLogOrVar("abc||def", true);
        ASSERT_EQUALS(true, Token::Match(varLogOrVar.tokens(), "%var% || %var%"));
    }

    void matchSingleChar() {
        givenACodeSampleToTokenize singleChar("a", true);
        ASSERT_EQUALS(true, Token::Match(singleChar.tokens(), "[a|bc]"));
        ASSERT_EQUALS(false, Token::Match(singleChar.tokens(), "[d|ef]"));

        Token multiChar(0);
        multiChar.str("[ab");
        ASSERT_EQUALS(false, Token::Match(&multiChar, "[ab|def]"));
    }

    void matchNothingOrAnyNotElse() {
        givenACodeSampleToTokenize emptyString("", true);
        ASSERT_EQUALS(true, Token::Match(emptyString.tokens(), "!!else"));
        ASSERT_EQUALS(false, Token::Match(emptyString.tokens(), "!!else something"));

        givenACodeSampleToTokenize ifSemicolon("if ;", true);
        ASSERT_EQUALS(true, Token::Match(ifSemicolon.tokens(), "!!return if"));
        ASSERT_EQUALS(true, Token::Match(ifSemicolon.tokens(), "if ; !!else"));

        givenACodeSampleToTokenize ifSemicolonSomething("if ; something", true);
        ASSERT_EQUALS(true, Token::Match(ifSemicolonSomething.tokens(), "if ; !!else"));

        givenACodeSampleToTokenize justElse("else", true);
        ASSERT_EQUALS(false, Token::Match(justElse.tokens(), "!!else"));

        givenACodeSampleToTokenize ifSemicolonElse("if ; else", true);
        ASSERT_EQUALS(false, Token::Match(ifSemicolonElse.tokens(), "if ; !!else"));
    }

    void matchType() {
        givenACodeSampleToTokenize type("abc", true);
        ASSERT_EQUALS(true, Token::Match(type.tokens(), "%type%"));

        givenACodeSampleToTokenize isVar("int a = 3 ;");
        ASSERT_EQUALS(true, Token::Match(isVar.tokens(), "%type%"));
        ASSERT_EQUALS(true, Token::Match(isVar.tokens(), "%type% %var%"));
        ASSERT_EQUALS(false, Token::Match(isVar.tokens(), "%type% %type%"));

        givenACodeSampleToTokenize noType("delete", true);
        ASSERT_EQUALS(false, Token::Match(noType.tokens(), "%type%"));
    }

    void matchChar() {
        givenACodeSampleToTokenize chr1("'a'", true);
        ASSERT_EQUALS(true, Token::Match(chr1.tokens(), "%char%"));

        givenACodeSampleToTokenize chr2("'1'", true);
        ASSERT_EQUALS(true, Token::Match(chr2.tokens(), "%char%"));

        givenACodeSampleToTokenize noChr("\"10\"", true);
        ASSERT_EQUALS(false, Token::Match(noChr.tokens(), "%char%"));
    }

    void matchCompOp() {
        givenACodeSampleToTokenize comp1("<=", true);
        ASSERT_EQUALS(true, Token::Match(comp1.tokens(), "%comp%"));

        givenACodeSampleToTokenize comp2(">", true);
        ASSERT_EQUALS(true, Token::Match(comp2.tokens(), "%comp%"));

        givenACodeSampleToTokenize noComp("=", true);
        ASSERT_EQUALS(false, Token::Match(noComp.tokens(), "%comp%"));
    }

    void matchStr() {
        givenACodeSampleToTokenize noStr1("abc", true);
        ASSERT_EQUALS(false, Token::Match(noStr1.tokens(), "%str%"));

        givenACodeSampleToTokenize noStr2("'a'", true);
        ASSERT_EQUALS(false, Token::Match(noStr2.tokens(), "%str%"));

        givenACodeSampleToTokenize str("\"abc\"", true);
        ASSERT_EQUALS(true, Token::Match(str.tokens(), "%str%"));

        // Empty string
        givenACodeSampleToTokenize emptyStr("\"\"", true);
        ASSERT_EQUALS(true, Token::Match(emptyStr.tokens(), "%str%"));
    }

    void matchVarid() {
        givenACodeSampleToTokenize var("int a ; int b ;");

        // Varid == 0 should throw exception
        ASSERT_THROW(Token::Match(var.tokens(), "%type% %varid% ; %type% %var%", 0),InternalError);

        ASSERT_EQUALS(true, Token::Match(var.tokens(), "%type% %varid% ; %type% %var%", 1));
        ASSERT_EQUALS(true, Token::Match(var.tokens(), "%type% %var% ; %type% %varid%", 2));

        // Try to match two different varids in one match call
        ASSERT_EQUALS(false, Token::Match(var.tokens(), "%type% %varid% ; %type% %varid%", 2));
    }

    void matchNumeric() {
        givenACodeSampleToTokenize nonNumeric("abc", true);
        ASSERT_EQUALS(false, Token::Match(nonNumeric.tokens(), "%num%"));

        givenACodeSampleToTokenize binary("101010b", true);
        ASSERT_EQUALS(true, Token::Match(binary.tokens(), "%num%"));

        givenACodeSampleToTokenize octal("0123", true);
        ASSERT_EQUALS(true, Token::Match(octal.tokens(), "%num%"));

        givenACodeSampleToTokenize decimal("4567", true);
        ASSERT_EQUALS(true, Token::Match(decimal.tokens(), "%num%"));

        givenACodeSampleToTokenize hexadecimal("0xDEADBEEF", true);
        ASSERT_EQUALS(true, Token::Match(hexadecimal.tokens(), "%num%"));

        givenACodeSampleToTokenize floatingPoint("0.0f", true);
        ASSERT_EQUALS(true, Token::Match(hexadecimal.tokens(), "%num%"));

        givenACodeSampleToTokenize doublePrecision("0.0d", true);
        ASSERT_EQUALS(true, Token::Match(hexadecimal.tokens(), "%num%"));

        givenACodeSampleToTokenize unsignedInt("0U", true);
        ASSERT_EQUALS(true, Token::Match(hexadecimal.tokens(), "%num%"));

        givenACodeSampleToTokenize unsignedLong("0UL", true);
        ASSERT_EQUALS(true, Token::Match(hexadecimal.tokens(), "%num%"));

        givenACodeSampleToTokenize unsignedLongLong("0ULL", true);
        ASSERT_EQUALS(true, Token::Match(hexadecimal.tokens(), "%num%"));

        givenACodeSampleToTokenize positive("+666", true);
        ASSERT_EQUALS(true, Token::Match(positive.tokens(), "+ %num%"));

        givenACodeSampleToTokenize negative("-42", true);
        ASSERT_EQUALS(true, Token::Match(negative.tokens(), "- %num%"));
    }


    void matchBoolean() {
        givenACodeSampleToTokenize yes("YES", true);
        ASSERT_EQUALS(false, Token::Match(yes.tokens(), "%bool%"));

        givenACodeSampleToTokenize positive("true", true);
        ASSERT_EQUALS(true, Token::Match(positive.tokens(), "%bool%"));

        givenACodeSampleToTokenize negative("false", true);
        ASSERT_EQUALS(true, Token::Match(negative.tokens(), "%bool%"));
    }

    void matchOr() {
        givenACodeSampleToTokenize bitwiseOr("|", true);
        ASSERT_EQUALS(true,  Token::Match(bitwiseOr.tokens(), "%or%"));
        ASSERT_EQUALS(true,  Token::Match(bitwiseOr.tokens(), "%op%"));
        ASSERT_EQUALS(false, Token::Match(bitwiseOr.tokens(), "%oror%"));

        givenACodeSampleToTokenize bitwiseOrAssignment("|=");
        ASSERT_EQUALS(false,  Token::Match(bitwiseOrAssignment.tokens(), "%or%"));
        ASSERT_EQUALS(false,  Token::Match(bitwiseOrAssignment.tokens(), "%op%"));
        ASSERT_EQUALS(false, Token::Match(bitwiseOrAssignment.tokens(), "%oror%"));

        givenACodeSampleToTokenize logicalOr("||", true);
        ASSERT_EQUALS(false, Token::Match(logicalOr.tokens(), "%or%"));
        ASSERT_EQUALS(true,  Token::Match(logicalOr.tokens(), "%op%"));
        ASSERT_EQUALS(true,  Token::Match(logicalOr.tokens(), "%oror%"));
        ASSERT_EQUALS(true,  Token::Match(logicalOr.tokens(), "&&|%oror%"));
        ASSERT_EQUALS(true,  Token::Match(logicalOr.tokens(), "%oror%|&&"));

        givenACodeSampleToTokenize logicalAnd("&&", true);
        ASSERT_EQUALS(true, Token::simpleMatch(logicalAnd.tokens(), "&&"));
        ASSERT_EQUALS(true, Token::Match(logicalAnd.tokens(), "&&|%oror%"));
        ASSERT_EQUALS(true, Token::Match(logicalAnd.tokens(), "%oror%|&&"));
    }

    void append_vector(std::vector<std::string> &dest, const std::vector<std::string> &src) const {
        dest.insert(dest.end(), src.begin(), src.end());
    }

    void initOps() {
        arithmeticalOps.push_back("+");
        arithmeticalOps.push_back("-");
        arithmeticalOps.push_back("*");
        arithmeticalOps.push_back("/");
        arithmeticalOps.push_back("%");
        arithmeticalOps.push_back("<<");
        arithmeticalOps.push_back(">>");

        logicalOps.push_back("&&");
        logicalOps.push_back("||");
        logicalOps.push_back("!");
        comparisionOps.push_back("==");
        comparisionOps.push_back("!=");
        comparisionOps.push_back("<");
        comparisionOps.push_back("<=");
        comparisionOps.push_back(">");
        comparisionOps.push_back(">=");
        bitOps.push_back("&");
        bitOps.push_back("|");
        bitOps.push_back("^");
        bitOps.push_back("~");

        extendedOps.push_back(",");
        extendedOps.push_back("[");
        extendedOps.push_back("]");
        extendedOps.push_back("(");
        extendedOps.push_back(")");
        extendedOps.push_back("?");
        extendedOps.push_back(":");

        assignmentOps.push_back("=");
        assignmentOps.push_back("+=");
        assignmentOps.push_back("-=");
        assignmentOps.push_back("*=");
        assignmentOps.push_back("/=");
        assignmentOps.push_back("%=");
        assignmentOps.push_back("&=");
        assignmentOps.push_back("^=");
        assignmentOps.push_back("|=");
        assignmentOps.push_back("<<=");
        assignmentOps.push_back(">>=");
    }

    void matchOp() {
        std::vector<std::string> test_ops;
        append_vector(test_ops, arithmeticalOps);
        append_vector(test_ops, bitOps);
        append_vector(test_ops, comparisionOps);
        append_vector(test_ops, logicalOps);

        std::vector<std::string>::const_iterator test_op, test_ops_end = test_ops.end();
        for (test_op = test_ops.begin(); test_op != test_ops_end; ++test_op) {
            ASSERT_EQUALS(true, Match(*test_op, "%op%"));
        }

        // Negative test against other operators
        std::vector<std::string> other_ops;
        append_vector(other_ops, extendedOps);
        append_vector(other_ops, assignmentOps);

        std::vector<std::string>::const_iterator other_op, other_ops_end = other_ops.end();
        for (other_op = other_ops.begin(); other_op != other_ops_end; ++other_op) {
            ASSERT_EQUALS_MSG(false, Match(*other_op, "%op%"), "Failing other operator: " + *other_op);
        }
    }

    void isArithmeticalOp() {
        std::vector<std::string>::const_iterator test_op, test_ops_end = arithmeticalOps.end();
        for (test_op = arithmeticalOps.begin(); test_op != test_ops_end; ++test_op) {
            Token tok(NULL);
            tok.str(*test_op);
            ASSERT_EQUALS(true, tok.isArithmeticalOp());
        }

        // Negative test against other operators
        std::vector<std::string> other_ops;
        append_vector(other_ops, bitOps);
        append_vector(other_ops, comparisionOps);
        append_vector(other_ops, logicalOps);
        append_vector(other_ops, extendedOps);
        append_vector(other_ops, assignmentOps);

        std::vector<std::string>::const_iterator other_op, other_ops_end = other_ops.end();
        for (other_op = other_ops.begin(); other_op != other_ops_end; ++other_op) {
            Token tok(NULL);
            tok.str(*other_op);
            ASSERT_EQUALS_MSG(false, tok.isArithmeticalOp(), "Failing arithmetical operator: " + *other_op);
        }
    }

    void isOp() {
        std::vector<std::string> test_ops;
        append_vector(test_ops, arithmeticalOps);
        append_vector(test_ops, bitOps);
        append_vector(test_ops, comparisionOps);
        append_vector(test_ops, logicalOps);

        std::vector<std::string>::const_iterator test_op, test_ops_end = test_ops.end();
        for (test_op = test_ops.begin(); test_op != test_ops_end; ++test_op) {
            Token tok(NULL);
            tok.str(*test_op);
            ASSERT_EQUALS(true, tok.isOp());
        }

        // Negative test against other operators
        std::vector<std::string> other_ops;
        append_vector(other_ops, extendedOps);
        append_vector(other_ops, assignmentOps);

        std::vector<std::string>::const_iterator other_op, other_ops_end = other_ops.end();
        for (other_op = other_ops.begin(); other_op != other_ops_end; ++other_op) {
            Token tok(NULL);
            tok.str(*other_op);
            ASSERT_EQUALS_MSG(false, tok.isOp(), "Failing normal operator: " + *other_op);
        }
    }

    void isExtendedOp() {
        std::vector<std::string> test_ops;
        append_vector(test_ops, arithmeticalOps);
        append_vector(test_ops, bitOps);
        append_vector(test_ops, comparisionOps);
        append_vector(test_ops, logicalOps);
        append_vector(test_ops, extendedOps);

        std::vector<std::string>::const_iterator test_op, test_ops_end = test_ops.end();
        for (test_op = test_ops.begin(); test_op != test_ops_end; ++test_op) {
            Token tok(NULL);
            tok.str(*test_op);
            ASSERT_EQUALS(true, tok.isExtendedOp());
        }

        // Negative test against assignment operators
        std::vector<std::string>::const_iterator other_op, other_ops_end = assignmentOps.end();
        for (other_op = assignmentOps.begin(); other_op != other_ops_end; ++other_op) {
            Token tok(NULL);
            tok.str(*other_op);
            ASSERT_EQUALS_MSG(false, tok.isExtendedOp(), "Failing assignment operator: " + *other_op);
        }
    }

    void isAssignmentOp() {
        std::vector<std::string>::const_iterator test_op, test_ops_end = assignmentOps.end();
        for (test_op = assignmentOps.begin(); test_op != test_ops_end; ++test_op) {
            Token tok(NULL);
            tok.str(*test_op);
            ASSERT_EQUALS(true, tok.isAssignmentOp());
        }

        // Negative test against other operators
        std::vector<std::string> other_ops;
        append_vector(other_ops, arithmeticalOps);
        append_vector(other_ops, bitOps);
        append_vector(other_ops, comparisionOps);
        append_vector(other_ops, logicalOps);
        append_vector(other_ops, extendedOps);

        std::vector<std::string>::const_iterator other_op, other_ops_end = other_ops.end();
        for (other_op = other_ops.begin(); other_op != other_ops_end; ++other_op) {
            Token tok(NULL);
            tok.str(*other_op);
            ASSERT_EQUALS_MSG(false, tok.isAssignmentOp(), "Failing assignment operator: " + *other_op);
        }
    }

    void operators() {
        std::vector<std::string>::const_iterator test_op;
        for (test_op = extendedOps.begin(); test_op != extendedOps.end(); ++test_op) {
            Token tok(NULL);
            tok.str(*test_op);
            ASSERT_EQUALS(Token::eExtendedOp, tok.type());
        }
        for (test_op = logicalOps.begin(); test_op != logicalOps.end(); ++test_op) {
            Token tok(NULL);
            tok.str(*test_op);
            ASSERT_EQUALS(Token::eLogicalOp, tok.type());
        }
        for (test_op = bitOps.begin(); test_op != bitOps.end(); ++test_op) {
            Token tok(NULL);
            tok.str(*test_op);
            ASSERT_EQUALS(Token::eBitOp, tok.type());
        }
        for (test_op = comparisionOps.begin(); test_op != comparisionOps.end(); ++test_op) {
            Token tok(NULL);
            tok.str(*test_op);
            ASSERT_EQUALS(Token::eComparisonOp, tok.type());
        }
        Token tok(NULL);
        tok.str("++");
        ASSERT_EQUALS(Token::eIncDecOp, tok.type());
        tok.str("--");
        ASSERT_EQUALS(Token::eIncDecOp, tok.type());
    }

    void literals() {
        Token tok(NULL);

        tok.str("\"foo\"");
        ASSERT(tok.type() == Token::eString);
        tok.str("\"\"");
        ASSERT(tok.type() == Token::eString);
        tok.str("'f'");
        ASSERT(tok.type() == Token::eChar);
        tok.str("12345");
        ASSERT(tok.type() == Token::eNumber);
        tok.str("-55");
        ASSERT(tok.type() == Token::eNumber);
        tok.str("true");
        ASSERT(tok.type() == Token::eBoolean);
        tok.str("false");
        ASSERT(tok.type() == Token::eBoolean);
    }

    void isStandardType() {
        std::vector<std::string> standard_types;
        standard_types.push_back("bool");
        standard_types.push_back("char");
        standard_types.push_back("short");
        standard_types.push_back("int");
        standard_types.push_back("long");
        standard_types.push_back("float");
        standard_types.push_back("double");
        standard_types.push_back("size_t");

        std::vector<std::string>::const_iterator test_op, test_ops_end = standard_types.end();
        for (test_op = standard_types.begin(); test_op != test_ops_end; ++test_op) {
            Token tok(NULL);
            tok.str(*test_op);
            ASSERT_EQUALS_MSG(true, tok.isStandardType(), "Failing standard type: " + *test_op);
        }

        // Negative test
        Token tok(0);
        tok.str("string");
        ASSERT_EQUALS(false, tok.isStandardType());

        // Change back to standard type
        tok.str("int");
        ASSERT_EQUALS(true, tok.isStandardType());
    }

    void updateProperties() {
        Token tok(NULL);
        tok.str("foobar");

        ASSERT_EQUALS(true, tok.isName());
        ASSERT_EQUALS(false, tok.isNumber());

        tok.str("123456");

        ASSERT_EQUALS(false, tok.isName());
        ASSERT_EQUALS(true, tok.isNumber());
    }

    void updatePropertiesConcatStr() {
        Token tok(NULL);
        tok.str("true");

        ASSERT_EQUALS(true, tok.isBoolean());

        tok.concatStr("123");

        ASSERT_EQUALS(false, tok.isBoolean());
        ASSERT_EQUALS("tru23", tok.str());
    }

    void isNameGuarantees1() {
        Token tok(NULL);
        tok.str("Name");
        ASSERT_EQUALS(true, tok.isName());
    }

    void isNameGuarantees2() {
        Token tok(NULL);
        tok.str("_name");
        ASSERT_EQUALS(true, tok.isName());
    }

    void isNameGuarantees3() {
        Token tok(NULL);
        tok.str("_123");
        ASSERT_EQUALS(true, tok.isName());
    }

    void isNameGuarantees4() {
        Token tok(NULL);
        tok.str("123456");
        ASSERT_EQUALS(false, tok.isName());
        ASSERT_EQUALS(true, tok.isNumber());
    }

    void isNameGuarantees5() {
        Token tok(NULL);
        tok.str("a123456");
        ASSERT_EQUALS(true, tok.isName());
        ASSERT_EQUALS(false, tok.isNumber());
    }


    void canFindMatchingBracketsNeedsOpen() {
        givenACodeSampleToTokenize var("std::deque<std::set<int> > intsets;");

        const Token* t = 0;
        bool found = var.tokens()->findClosingBracket(t);
        ASSERT(! found);
        ASSERT(! t);
    }

    void canFindMatchingBracketsInnerPair() {
        givenACodeSampleToTokenize var("std::deque<std::set<int> > intsets;");

        Token* t = 0;
        bool found = var.tokens()->tokAt(7)->findClosingBracket(t);
        ASSERT(found);
        ASSERT_EQUALS(">", t->str());
        ASSERT(var.tokens()->tokAt(9) == t);
    }

    void canFindMatchingBracketsOuterPair() {
        givenACodeSampleToTokenize var("std::deque<std::set<int> > intsets;");

        const Token* t = 0;
        bool found = var.tokens()->tokAt(3)->findClosingBracket(t);
        ASSERT(found);
        ASSERT_EQUALS(">", t->str());
        ASSERT(var.tokens()->tokAt(10) == t);

    }

    void canFindMatchingBracketsWithTooManyClosing() {
        givenACodeSampleToTokenize var("X< 1>2 > x1;\n");

        const Token* t = 0;
        bool found = var.tokens()->next()->findClosingBracket(t);
        ASSERT(found);
        ASSERT_EQUALS(">", t->str());
        ASSERT(var.tokens()->tokAt(3) == t);
    }

    void canFindMatchingBracketsWithTooManyOpening() {
        givenACodeSampleToTokenize var("X < (2 < 1) > x1;\n");

        const Token* t = 0;
        bool found = var.tokens()->next()->findClosingBracket(t);
        ASSERT(found);

        found = var.tokens()->tokAt(4)->findClosingBracket(t);
        ASSERT(!found);
    }

};

REGISTER_TEST(TestToken)
