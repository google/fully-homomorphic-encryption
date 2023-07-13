#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_RUST_TFHE_RS_TEMPLATES_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_RUST_TFHE_RS_TEMPLATES_H_

#include "absl/strings/string_view.h"

namespace fhe {
namespace rust {
namespace transpiler {

// Constants used for string templating, these are placed here so the tests
// can reuse them.
constexpr absl::string_view kCodegenTemplate = R"rust(
use rayon::prelude::*;
use std::collections::HashMap;
use tfhe::shortint;
use tfhe::shortint::prelude::*;
use tfhe::shortint::CiphertextBig as Ciphertext;

fn generate_lut(lut_as_int: u64, server_key: &ServerKey) -> shortint::server_key::LookupTableOwned {
    let f = |x: u64| (lut_as_int >> (x as u8)) & 1;
    return server_key.generate_accumulator(f);
}

enum GateInput {
    Arg(usize, usize), // arg + index
    Output(usize), // reuse of output wire
    Tv(usize),  // temp value
    Cst(bool),  // constant
}

use GateInput::*;

$gate_levels

pub fn $function_signature {
    let constant_false: Ciphertext = server_key.create_trivial(0);
    let constant_true: Ciphertext = server_key.create_trivial(1);
    let args: &[&Vec<Ciphertext>] = &[$ordered_params];

    let luts = {
        let mut luts: HashMap<u64, shortint::server_key::LookupTableOwned> = HashMap::new();
        const LUTS_AS_INTS: [u64; $num_luts] = [$comma_separated_luts];
        for lut_as_int in LUTS_AS_INTS {
            luts.insert(lut_as_int, generate_lut(lut_as_int, server_key));
        }
        luts
    };

    let lut3 = |args: &[&Ciphertext], lut: u64| -> Ciphertext {
        let top_bit = server_key.unchecked_scalar_mul(args[2], 4);
        let middle_bit = server_key.unchecked_scalar_mul(args[1], 2);
        let ct_input = server_key.unchecked_add(&top_bit, &server_key.unchecked_add(&middle_bit, args[0]));
        return server_key.apply_lookup_table(&ct_input, &luts[&lut]);
    };

    let mut temp_nodes = Vec::new();
    temp_nodes.resize($total_num_gates, constant_false.clone());
    let mut $output_stem = Vec::new();
    $output_stem.resize($num_outputs, constant_false.clone());

    let mut run_level = |tasks: &[((usize, bool, u64), &[GateInput])]| {
        let updates = tasks
            .into_par_iter()
            .map(|(k, task_args)| {
                let (id, is_output, lut) = *k;
                let task_args = task_args.into_iter()
                  .map(|arg| match arg {
                    Cst(false) => &constant_false,
                    Cst(true) => &constant_true,
                    Arg(pos, ndx) => &args[*pos][*ndx],
                    Tv(ndx) => &temp_nodes[*ndx],
                    Output(ndx) => &$output_stem[*ndx],
                  }).collect::<Vec<_>>();
                ((id, is_output), lut3(&task_args, lut))
            })
            .collect::<Vec<_>>();
        updates.into_iter().for_each(|(k, v)| {
            let (index, is_output) = k;
            if is_output {
                $output_stem[index] = v;
            } else {
                temp_nodes[index] = v;
            }
        });
    };
$run_level_ops

$output_assignment_block

    $return_statement
}
)rust";

// The data of a level's gate ops must be defined statically, or else the
// rustc (LLVM) optimizing passes will take forever.
//
// e.g.,
//
//   const LEVEL_3: [((usize, bool, u64), &[GateInput]); 8] = [
//       ((1, true, 6), &[Arg(0, 0), Arg(0, 1), Cst(false)]),
//       ((2, true, 120), &[Arg(0, 0), Arg(0, 1), Arg(0, 2)]),
//       ...
//   ];
//
constexpr absl::string_view kLevelTemplate = R"rust(
const LEVEL_%d: [((usize, bool, u64), &[GateInput]); %d] = [
%s
];)rust";

// A single gate operation defined as constant data
//
//   ((output_id, is_output, lut_defn), &[arglist]),
//
// e.g.,
//
//   ((2, false, 120), &[Arg(0, 0), Arg(0, 1), Arg(0, 2)]),
//
constexpr absl::string_view kTaskTemplate = "    ((%d, %s, %d), &[%s]),";

// The commands used to run the levels defined by kLevelTemplate above.
//
// e.g.,
//
//  run_level(&LEVEL_0);
//  run_level(&LEVEL_1);
//  ...
//
constexpr absl::string_view kRunLevelTemplate = "    run_level(&LEVEL_%d);";

// This is needed for the output section of a program,
// when the output values from the yosys netlist must be split off into multiple
// output values for the caller of the code-genned function.
//
// e.g., the following writes the first 16 bits of `out` to an outparameter,
// then returns the remaining bits.
//
//     outputValue[0] = out[0].clone();
//     outputValue[1] = out[1].clone();
//     outputValue[2] = out[2].clone();
//     outputValue[3] = out[3].clone();
//     outputValue[4] = out[4].clone();
//     outputValue[5] = out[5].clone();
//     outputValue[6] = out[6].clone();
//     outputValue[7] = out[7].clone();
//     outputValue[8] = out[8].clone();
//     outputValue[9] = out[9].clone();
//     outputValue[10] = out[10].clone();
//     outputValue[11] = out[11].clone();
//     outputValue[12] = out[12].clone();
//     outputValue[13] = out[13].clone();
//     outputValue[14] = out[14].clone();
//     outputValue[15] = out[15].clone();
//     out = out.split_off(16);
//
constexpr absl::string_view kAssignmentTemplate = "    %s[%d] = %s.clone();";
constexpr absl::string_view kSplitTemplate = "    %s = %s.split_off(%d);";

}  // namespace transpiler
}  // namespace rust
}  // namespace fhe

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_RUST_TFHE_RS_TEMPLATES_H_
