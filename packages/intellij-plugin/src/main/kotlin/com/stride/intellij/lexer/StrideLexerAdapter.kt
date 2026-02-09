package com.stride.intellij.lexer

import com.intellij.lexer.FlexAdapter

class StrideLexerAdapter : FlexAdapter(StrideLexer(null))
