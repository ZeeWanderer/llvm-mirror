//===-- llvm/MC/MCAsmLexer.h - Abstract Asm Lexer Interface -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MC_MCASMLEXER_H
#define LLVM_MC_MCASMLEXER_H

#include "llvm/ADT/StringRef.h"
#include "llvm/System/DataTypes.h"

namespace llvm {
class MCAsmLexer;
class MCInst;
class SMLoc;
class Target;

/// AsmToken - Target independent representation for an assembler token.
struct AsmToken {
  enum TokenKind {
    // Markers
    Eof, Error,

    // String values.
    Identifier,
    String,
    
    // Integer values.
    Integer,
    
    // No-value.
    EndOfStatement,
    Colon,
    Plus, Minus, Tilde,
    Slash,    // '/'
    LParen, RParen, LBrac, RBrac, LCurly, RCurly,
    Star, Comma, Dollar, Equal, EqualEqual,
    
    Pipe, PipePipe, Caret, 
    Amp, AmpAmp, Exclaim, ExclaimEqual, Percent, Hash,
    Less, LessEqual, LessLess, LessGreater,
    Greater, GreaterEqual, GreaterGreater
  };

  TokenKind Kind;

  /// A reference to the entire token contents; this is always a pointer into
  /// a memory buffer owned by the source manager.
  StringRef Str;

  int64_t IntVal;

public:
  AsmToken() {}
  AsmToken(TokenKind _Kind, const StringRef &_Str, int64_t _IntVal = 0)
    : Kind(_Kind), Str(_Str), IntVal(_IntVal) {}

  TokenKind getKind() const { return Kind; }
  bool is(TokenKind K) const { return Kind == K; }
  bool isNot(TokenKind K) const { return Kind != K; }

  SMLoc getLoc() const;

  /// getStringContents - Get the contents of a string token (without quotes).
  StringRef getStringContents() const { 
    assert(Kind == String && "This token isn't a string!");
    return Str.slice(1, Str.size() - 1);
  }

  /// getIdentifier - Get the identifier string for the current token, which
  /// should be an identifier or a string. This gets the portion of the string
  /// which should be used as the identifier, e.g., it does not include the
  /// quotes on strings.
  StringRef getIdentifier() const {
    if (Kind == Identifier)
      return getString();
    return getStringContents();
  }

  /// getString - Get the string for the current token, this includes all
  /// characters (for example, the quotes on strings) in the token.
  ///
  /// The returned StringRef points into the source manager's memory buffer, and
  /// is safe to store across calls to Lex().
  StringRef getString() const { return Str; }

  // FIXME: Don't compute this in advance, it makes every token larger, and is
  // also not generally what we want (it is nicer for recovery etc. to lex 123br
  // as a single token, then diagnose as an invalid number).
  int64_t getIntVal() const { 
    assert(Kind == Integer && "This token isn't an integer!");
    return IntVal; 
  }
};

/// MCAsmLexer - Generic assembler lexer interface, for use by target specific
/// assembly lexers.
class MCAsmLexer {
  /// The current token, stored in the base class for faster access.
  AsmToken CurTok;

  MCAsmLexer(const MCAsmLexer &);   // DO NOT IMPLEMENT
  void operator=(const MCAsmLexer &);  // DO NOT IMPLEMENT
protected: // Can only create subclasses.
  MCAsmLexer();

  virtual AsmToken LexToken() = 0;

public:
  virtual ~MCAsmLexer();

  /// Lex - Consume the next token from the input stream and return it.
  ///
  /// The lexer will continuosly return the end-of-file token once the end of
  /// the main input file has been reached.
  const AsmToken &Lex() {
    return CurTok = LexToken();
  }

  /// getTok - Get the current (last) lexed token.
  const AsmToken &getTok() {
    return CurTok;
  }

  /// getKind - Get the kind of current token.
  AsmToken::TokenKind getKind() const { return CurTok.getKind(); }

  /// is - Check if the current token has kind \arg K.
  bool is(AsmToken::TokenKind K) const { return CurTok.is(K); }

  /// isNot - Check if the current token has kind \arg K.
  bool isNot(AsmToken::TokenKind K) const { return CurTok.isNot(K); }
};

} // End llvm namespace

#endif
