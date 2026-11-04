#include <gtest/gtest.h>
#include "raven/lexer.hpp"

using namespace raven;

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

TEST(LexerTest, Keywords) {
    Lexer lexer("let if else while print fn return true false");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 10);
    EXPECT_EQ(tokens[0].type, TokenType::Let);
    EXPECT_EQ(tokens[1].type, TokenType::If);
    EXPECT_EQ(tokens[2].type, TokenType::Else);
    EXPECT_EQ(tokens[3].type, TokenType::While);
    EXPECT_EQ(tokens[4].type, TokenType::Print);
    EXPECT_EQ(tokens[5].type, TokenType::Fn);
    EXPECT_EQ(tokens[6].type, TokenType::Return);
    EXPECT_EQ(tokens[7].type, TokenType::True);
    EXPECT_EQ(tokens[8].type, TokenType::False);
    EXPECT_EQ(tokens[9].type, TokenType::EndOfFile);
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
    Lexer lexer("0 20 12345 3.14");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0], (Token{TokenType::Number, "0", 1, 1}));
    EXPECT_EQ(tokens[1], (Token{TokenType::Number, "20", 1, 3}));
    EXPECT_EQ(tokens[2], (Token{TokenType::Number, "12345", 1, 6}));
    EXPECT_EQ(tokens[3], (Token{TokenType::Number, "3.14", 1, 12}));
    EXPECT_EQ(tokens[4].type, TokenType::EndOfFile);
}

TEST(LexerTest, StringsWithEscapes) {
    Lexer lexer("\"Hello, World!\" \"Ran\\nCohen\"");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], (Token{TokenType::String, "Hello, World!", 1, 1}));
    EXPECT_EQ(tokens[1], (Token{TokenType::String, "Ran\nCohen", 1, 17}));
    EXPECT_EQ(tokens[2].type, TokenType::EndOfFile);
}

TEST(LexerTest, SingleAndTwoCharOperators) {
    Lexer lexer("= == ! != < <= > >= + - * / % && ||");
    auto tokens = lexer.tokenize();

    std::vector<TokenType> expected = {
        TokenType::Equal,
        TokenType::EqualEqual,
        TokenType::Bang,
        TokenType::BangEqual,
        TokenType::Less,
        TokenType::LessEqual,
        TokenType::Greater,
        TokenType::GreaterEqual,
        TokenType::Plus,
        TokenType::Minus,
        TokenType::Star,
        TokenType::Slash,
        TokenType::Percent,
        TokenType::AmpAmp,
        TokenType::PipePipe,
        TokenType::EndOfFile
    };

    ASSERT_EQ(tokens.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]);
    }
}

TEST(LexerTest, CommentsSkipped) {
    std::string source =
        "// this is a comment\n"
        "let x = 10; /* multi\n"
        "line */ let y = 20;";

    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[0], (Token{TokenType::Let, "let", 2, 1}));
    EXPECT_EQ(tokens[1], (Token{TokenType::Identifier, "x", 2, 5}));
    EXPECT_EQ(tokens[2], (Token{TokenType::Equal, "=", 2, 7}));
    EXPECT_EQ(tokens[3], (Token{TokenType::Number, "10", 2, 9}));
    EXPECT_EQ(tokens[4], (Token{TokenType::Semicolon, ";", 2, 11}));

    EXPECT_EQ(tokens[5], (Token{TokenType::Let, "let", 3, 9}));
    EXPECT_EQ(tokens[6], (Token{TokenType::Identifier, "y", 3, 13}));
    EXPECT_EQ(tokens[7], (Token{TokenType::Equal, "=", 3, 15}));
    EXPECT_EQ(tokens[8], (Token{TokenType::Number, "20", 3, 17}));
    EXPECT_EQ(tokens[9], (Token{TokenType::Semicolon, ";", 3, 19}));
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

TEST(LexerTest, ThisKeyword) {
    Lexer lexer("this data");
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::This);
    EXPECT_EQ(tokens[0].lexeme, "this");
    EXPECT_EQ(tokens[1].type, TokenType::Identifier);
    EXPECT_EQ(tokens[2].type, TokenType::EndOfFile);
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
