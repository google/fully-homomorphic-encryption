module attributes {tf_saved_model.semantics, tfl.description = "MLIR Converted.", tfl.schema_version = 3 : i32} {
  func.func @main(%arg0: tensor<1x1xi8> {tf_saved_model.index_path = ["dense_input"]}) -> (tensor<1x1xi8> {tf_saved_model.index_path = ["dense_2"]}) attributes {tf.entry_function = {inputs = "serving_default_dense_input:0", outputs = "StatefulPartitionedCall:0"}, tf_saved_model.exported_names = ["serving_default"]} {
    %0 = "tosa.const"() <{value = dense<429> : tensor<1xi32>}> : () -> tensor<1xi32>
    %1 = "tosa.const"() <{value = dense<[[-39, 59, 39, 21, 28, -32, -34, -35, 15, 27, -59, -41, 18, -35, -7, 127]]> : tensor<1x16xi8>}> : () -> tensor<1x16xi8>
    %2 = "tosa.const"() <{value = dense<[-729, 1954, 610, 0, 241, -471, -35, -867, 571, 581, 4260, 3943, 591, 0, -889, -5103]> : tensor<16xi32>}> : () -> tensor<16xi32>
    %3 = "tosa.const"() <{value = dense<"0xF41AED091921F424E021EFBCF7F5FA1903DCD20206F9F402FFFAEFF1EFD327E1FB27DDEBDBE4051A17FC241215EF1EE410FE14DA1CF8F3F1EFE2F309E3E9EDE3E415070B041B1AFEEB01DE21E60BEC03230A22241E2703E60324FFC011F8FCF1110CF5E0F30717E5E8EDFADCE823FB07DDFBFD0014261117E7F111EA0226040425211D0ADB1DDC2001FAE3370BF11A16EF1CE703E01602032118092ED9E5140BEA1AFCD81300C4D8ECD9FE0D1920D8D6E21FE9D7CAE2DDC613E7043E000114C7DBE71515F506D61ADC0922FE080213EF191EE209FDF314DDDA20D90FE3F9F7EEE924E629000716E21E0D23D3DDF714FA0822262109080F0BE012F47FDC58E526"> : tensor<16x16xi8>}> : () -> tensor<16x16xi8>
    %4 = "tosa.const"() <{value = dense<[0, 0, -5438, -5515, -1352, -1500, -4152, -84, 3396, 0, 1981, -5581, 0, -6964, 3407, -7217]> : tensor<16xi32>}> : () -> tensor<16xi32>
    %5 = "tosa.const"() <{value = dense<[[-9], [-54], [57], [71], [104], [115], [98], [99], [64], [-26], [127], [25], [-82], [68], [95], [86]]> : tensor<16x1xi8>}> : () -> tensor<16x1xi8>
    %6 = "tosa.fully_connected"(%arg0, %5, %4) <{quantization_info = #tosa.conv_quant<input_zp = -128, weight_zp = 0>}> : (tensor<1x1xi8>, tensor<16x1xi8>, tensor<16xi32>) -> tensor<1x16xi32>
    %7 = "tosa.rescale"(%6) <{double_round = true, input_zp = 0 : i32, multiplier = array<i32: 2039655736>, output_zp = -128 : i32, per_channel = false, scale32 = true, shift = array<i32: 38>}> : (tensor<1x16xi32>) -> tensor<1x16xi8>
    %8 = "tosa.clamp"(%7) <{max_fp = 0.000000e+00 : f32, max_int = 127 : i64, min_fp = 0.000000e+00 : f32, min_int = -128 : i64}> : (tensor<1x16xi8>) -> tensor<1x16xi8>
    %9 = "tosa.fully_connected"(%8, %3, %2) <{quantization_info = #tosa.conv_quant<input_zp = -128, weight_zp = 0>}> : (tensor<1x16xi8>, tensor<16x16xi8>, tensor<16xi32>) -> tensor<1x16xi32>
    %10 = "tosa.rescale"(%9) <{double_round = true, input_zp = 0 : i32, multiplier = array<i32: 1561796795>, output_zp = -128 : i32, per_channel = false, scale32 = true, shift = array<i32: 37>}> : (tensor<1x16xi32>) -> tensor<1x16xi8>
    %11 = "tosa.clamp"(%10) <{max_fp = 0.000000e+00 : f32, max_int = 127 : i64, min_fp = 0.000000e+00 : f32, min_int = -128 : i64}> : (tensor<1x16xi8>) -> tensor<1x16xi8>
    %12 = "tosa.fully_connected"(%11, %1, %0) <{quantization_info = #tosa.conv_quant<input_zp = -128, weight_zp = 0>}> : (tensor<1x16xi8>, tensor<1x16xi8>, tensor<1xi32>) -> tensor<1x1xi32>
    %13 = "tosa.rescale"(%12) <{double_round = true, input_zp = 0 : i32, multiplier = array<i32: 1630361836>, output_zp = 5 : i32, per_channel = false, scale32 = true, shift = array<i32: 36>}> : (tensor<1x1xi32>) -> tensor<1x1xi8>
    return %13 : tensor<1x1xi8>
  }
}
