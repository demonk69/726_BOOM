# Directive Audit

This T0 review changed no functional source and added no HLS directives. Existing source directives, including current INLINE/UNROLL and top/reset PIPELINE pragmas, are baseline facts and were not edited. T0 and the recommended T0R prohibit new pipeline stages, DATAFLOW, INLINE, UNROLL, false DEPENDENCE, or complete ARRAY_PARTITION as a timing shortcut.

`T0_NEW_HLS_SCHEDULING_DIRECTIVES=0`
