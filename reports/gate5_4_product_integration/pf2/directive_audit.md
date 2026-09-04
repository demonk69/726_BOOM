# PF2 Directive Audit

`S0_NO_NEW_DIRECTIVE=true`

The staged PF2 source diff adds no inline or unroll pragmas and no corresponding Tcl directives. The three directives remaining in `src/frontend.cpp` are the pre-existing inline directives at current lines 27, 40, and 55; all precede the PF2 integration code. Generated `src/boom_core_merged.cpp` contains only directives inherited from its canonical modular inputs.

The repair keeps the fixed two-lane packet precedence and admission logic as explicit expressions rather than adding synthesis directives.
