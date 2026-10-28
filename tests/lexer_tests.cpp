#include <gtest/gtest.h>
#include "minilang/lexer.hpp"

using namespace minilang;

TEST(LexerTest, EmptySource) {
    Lexer lexer("");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TokenType::EndOfFile);
    EXPECT_EQ(tokens[0].lexeme, "");
    EXPECT_EQ(tokens[0].line, 1);
    EXPECT_EQ(tokens[0].column, 1);
}

TEST(LexerTest, WhitespaceOnly) {
    Lexer lexer("   \t  \n   ");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TokenType::EndOfFile);
    EXPECT_EQ(tokens[0].line, 2);
    EXPECT_EQ(tokens[0].column, 4);
}

TEST(LexerTest, LetKeyword) {
    Lexer lexer("let");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 2);
    EXPECT_EQ(tokens[0].type, TokenType::Let);
    EXPECT_EQ(tokens[0].lexeme, "let");
    EXPECT_EQ(tokens[0].line, 1);
    EXPECT_EQ(tokens[0].column, 1);

    EXPECT_EQ(tokens[1].type, TokenType::EndOfFile);
}

TEST(LexerTest, Identifiers) {
    Lexer lexer("age foo_bar x123 letter let_var");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[0], (Token{TokenType::Identifier, "age", 1, 1}));
    EXPECT_EQ(tokens[1], (Token{TokenType::Identifier, "foo_bar", 1, 5}));
    EXPECT_EQ(tokens[2], (Token{TokenType::Identifier, "x123", 1, 13}));
    EXPECT_EQ(tokens[3], (Token{TokenType::Identifier, "letter", 1, 18}));
    EXPECT_EQ(tokens[4], (Token{TokenType::Identifier, "let_var", 1, 25}));
    EXPECT_EQ(tokens[5].type, TokenType::EndOfFile);
}

TEST(LexerTest, Numbers) {
    Lexer lexer("0 20 12345");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0], (Token{TokenType::Number, "0", 1, 1}));
    EXPECT_EQ(tokens[1], (Token{TokenType::Number, "20", 1, 3}));
    EXPECT_EQ(tokens[2], (Token{TokenType::Number, "12345", 1, 6}));
    EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
}

TEST(LexerTest, OperatorsAndPunctuation) {
    Lexer lexer("+ - * / = ;");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0], (Token{TokenType::Plus, "+", 1, 1}));
    EXPECT_EQ(tokens[1], (Token{TokenType::Minus, "-", 1, 3}));
    EXPECT_EQ(tokens[2], (Token{TokenType::Star, "*", 1, 5}));
    EXPECT_EQ(tokens[3], (Token{TokenType::Slash, "/", 1, 7}));
    EXPECT_EQ(tokens[4], (Token{TokenType::Equal, "=", 1, 9}));
    EXPECT_EQ(tokens[5], (Token{TokenType::Semicolon, ";", 1, 11}));
    EXPECT_EQ(tokens[6].type, TokenType::EndOfFile);
}

TEST(LexerTest, FullLetStatement) {
    Lexer lexer("let age = 20;");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[0], (Token{TokenType::Let, "let", 1, 1}));
    EXPECT_EQ(tokens[1], (Token{TokenType::Identifier, "age", 1, 5}));
    EXPECT_EQ(tokens[2], (Token{TokenType::Equal, "=", 1, 9}));
    EXPECT_EQ(tokens[3], (Token{TokenType::Number, "20", 1, 11}));
    EXPECT_EQ(tokens[4], (Token{TokenType::Semicolon, ";", 1, 13}));
    EXPECT_EQ(tokens[5].type, TokenType::EndOfFile);
}

TEST(LexerTest, MultilineTracking) {
    std::string source =
        "let a = 10;\n"
        "let b = 20;\n"
        "let c = a + b;";

    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    // Line 1: let a = 10; (5 tokens)
    EXPECT_EQ(tokens[0], (Token{TokenType::Let, "let", 1, 1}));
    EXPECT_EQ(tokens[1], (Token{TokenType::Identifier, "a", 1, 5}));
    EXPECT_EQ(tokens[2], (Token{TokenType::Equal, "=", 1, 7}));
    EXPECT_EQ(tokens[3], (Token{TokenType::Number, "10", 1, 9}));
    EXPECT_EQ(tokens[4], (Token{TokenType::Semicolon, ";", 1, 11}));

    // Line 2: let b = 20; (5 tokens)
    EXPECT_EQ(tokens[5], (Token{TokenType::Let, "let", 2, 1}));
    EXPECT_EQ(tokens[6], (Token{TokenType::Identifier, "b", 2, 5}));
    EXPECT_EQ(tokens[7], (Token{TokenType::Equal, "=", 2, 7}));
    EXPECT_EQ(tokens[8], (Token{TokenType::Number, "20", 2, 9}));
    EXPECT_EQ(tokens[9], (Token{TokenType::Semicolon, ";", 2, 11}));

    // Line 3: let c = a + b; (7 tokens)
    EXPECT_EQ(tokens[10], (Token{TokenType::Let, "let", 3, 1}));
    EXPECT_EQ(tokens[11], (Token{TokenType::Identifier, "c", 3, 5}));
    EXPECT_EQ(tokens[12], (Token{TokenType::Equal, "=", 3, 7}));
    EXPECT_EQ(tokens[13], (Token{TokenType::Identifier, "a", 3, 9}));
    EXPECT_EQ(tokens[14], (Token{TokenType::Plus, "+", 3, 11}));
    EXPECT_EQ(tokens[15], (Token{TokenType::Identifier, "b", 3, 13}));
    EXPECT_EQ(tokens[16], (Token{TokenType::Semicolon, ";", 3, 14}));

    // End of file
    EXPECT_EQ(tokens[17].type, TokenType::EndOfFile);
    EXPECT_EQ(tokens[17].line, 3);
}

TEST(LexerTest, InvalidCharacters) {
    Lexer lexer("@ # $");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0], (Token{TokenType::Invalid, "@", 1, 1}));
    EXPECT_EQ(tokens[1], (Token{TokenType::Invalid, "#", 1, 3}));
    EXPECT_EQ(tokens[2], (Token{TokenType::Invalid, "$", 1, 5}));
    EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
}
