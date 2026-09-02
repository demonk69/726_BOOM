# Call And Return Policy

The frozen P0 link-register convention is x1/x5. These flags are metadata proof only; P1 implements no RAS state.

| Form | is_call | is_return | Reason |
|---|---:|---:|---|
| JAL rd=x1 | true | false | writes link register x1 |
| JAL rd=x5 | true | false | writes alternate link register x5 |
| JAL other rd | false | false | no recognized link destination |
| JALR rd=x1/x5 | true | false | writes a recognized link register |
| JALR rd=x0, rs1=x1/x5, imm=0 | false | true | frozen return form |
| Other JALR | false | false | ordinary indirect jump unless rd is x1/x5 |
| C.JR x1/x5 | false | true | canonical JALR x0, link, 0 |
| C.JR other | false | false | ordinary indirect jump |
| C.JALR | true | false | canonical JALR x1, rs1, 0; architectural link remains original PC+2 |

Predecode does not calculate a link value. `instruction_length=2` preserves the information needed for later C.JALR link semantics.
