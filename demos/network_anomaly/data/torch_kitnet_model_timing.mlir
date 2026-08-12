#map = affine_map<(d0, d1) -> (d0, d1)>
#map1 = affine_map<(d0, d1) -> (d1)>
#map2 = affine_map<(d0, d1) -> (d0)>
module {
  func.func @main(%arg0: tensor<1x5xf32> {secret.secret}) -> (tensor<1xf32>, tensor<1x5xf32>) {
    debug.validate %arg0 {name = "input", metadata = "input"} : tensor<1x5xf32>
    %c2_i64 = arith.constant 2 : i64
    %cst = arith.constant 0.000000e+00 : f32
    %cst_0 = arith.constant 1.000000e+00 : f32
    %cst_1 = arith.constant dense_resource<torch_tensor_5_torch.float32_1> : tensor<5xf32>
    %cst_2 = arith.constant dense_resource<torch_tensor_5_1_torch.float32> : tensor<5x1xf32>
    %cst_3 = arith.constant dense<0.207050383> : tensor<1xf32>
    %cst_4 = arith.constant dense_resource<torch_tensor_1_5_torch.float32> : tensor<1x5xf32>
    %cst_5 = arith.constant dense_resource<torch_tensor_5_torch.float32> : tensor<5xf32>
    %cst_6 = arith.constant dense_resource<torch_tensor_5_4_torch.float32> : tensor<5x4xf32>
    %cst_7 = arith.constant dense_resource<torch_tensor_4_torch.float32> : tensor<4xf32>
    %cst_8 = arith.constant dense_resource<torch_tensor_4_5_torch.float32> : tensor<4x5xf32>
    %0 = tensor.empty() : tensor<5x4xf32>
    %transposed = linalg.transpose ins(%cst_8 : tensor<4x5xf32>) outs(%0 : tensor<5x4xf32>) permutation = [1, 0]
    %1 = tensor.empty() : tensor<1x4xf32>
    %2 = linalg.fill ins(%cst : f32) outs(%1 : tensor<1x4xf32>) -> tensor<1x4xf32>
    %3 = linalg.matmul ins(%arg0, %transposed : tensor<1x5xf32>, tensor<5x4xf32>) outs(%2 : tensor<1x4xf32>) -> tensor<1x4xf32>
    debug.validate %3 {name = "layer1_matmul", metadata = "layer1_matmul"} : tensor<1x4xf32>
    %4 = linalg.generic {indexing_maps = [#map, #map1, #map], iterator_types = ["parallel", "parallel"]} ins(%3, %cst_7 : tensor<1x4xf32>, tensor<4xf32>) outs(%1 : tensor<1x4xf32>) {
    ^bb0(%in: f32, %in_12: f32, %out: f32):
      %27 = arith.addf %in, %in_12 : f32
      linalg.yield %27 : f32
    } -> tensor<1x4xf32>
    debug.validate %4 {name = "layer1_bias", metadata = "layer1_bias"} : tensor<1x4xf32>
    %5 = linalg.generic {indexing_maps = [#map, #map], iterator_types = ["parallel", "parallel"]} ins(%4 : tensor<1x4xf32>) outs(%1 : tensor<1x4xf32>) {
    ^bb0(%in: f32, %out: f32):
      %27 = arith.negf %in : f32
      %28 = math.exp %27 : f32
      %29 = arith.addf %28, %cst_0 : f32
      %30 = arith.divf %cst_0, %29 : f32
      linalg.yield %30 : f32
    } -> tensor<1x4xf32>
    debug.validate %5 {name = "layer1_sigmoid", metadata = "layer1_sigmoid"} : tensor<1x4xf32>
    %6 = tensor.empty() : tensor<4x5xf32>
    %transposed_9 = linalg.transpose ins(%cst_6 : tensor<5x4xf32>) outs(%6 : tensor<4x5xf32>) permutation = [1, 0]
    %7 = tensor.empty() : tensor<1x5xf32>
    %8 = linalg.fill ins(%cst : f32) outs(%7 : tensor<1x5xf32>) -> tensor<1x5xf32>
    %9 = linalg.matmul ins(%5, %transposed_9 : tensor<1x4xf32>, tensor<4x5xf32>) outs(%8 : tensor<1x5xf32>) -> tensor<1x5xf32>
    debug.validate %9 {name = "layer2_matmul", metadata = "layer2_matmul"} : tensor<1x5xf32>
    %10 = linalg.generic {indexing_maps = [#map, #map1, #map], iterator_types = ["parallel", "parallel"]} ins(%9, %cst_5 : tensor<1x5xf32>, tensor<5xf32>) outs(%7 : tensor<1x5xf32>) {
    ^bb0(%in: f32, %in_12: f32, %out: f32):
      %27 = arith.addf %in, %in_12 : f32
      linalg.yield %27 : f32
    } -> tensor<1x5xf32>
    debug.validate %10 {name = "layer2_bias", metadata = "layer2_bias"} : tensor<1x5xf32>
    %11 = linalg.generic {indexing_maps = [#map, #map], iterator_types = ["parallel", "parallel"]} ins(%10 : tensor<1x5xf32>) outs(%7 : tensor<1x5xf32>) {
    ^bb0(%in: f32, %out: f32):
      %27 = arith.negf %in : f32
      %28 = math.exp %27 : f32
      %29 = arith.addf %28, %cst_0 : f32
      %30 = arith.divf %cst_0, %29 : f32
      linalg.yield %30 : f32
    } -> tensor<1x5xf32>
    debug.validate %11 {name = "layer2_sigmoid", metadata = "layer2_sigmoid"} : tensor<1x5xf32>
    %12 = linalg.generic {indexing_maps = [#map, #map, #map], iterator_types = ["parallel", "parallel"]} ins(%arg0, %11 : tensor<1x5xf32>, tensor<1x5xf32>) outs(%7 : tensor<1x5xf32>) {
    ^bb0(%in: f32, %in_12: f32, %out: f32):
      %27 = arith.subf %in, %in_12 : f32
      linalg.yield %27 : f32
    } -> tensor<1x5xf32>
    debug.validate %12 {name = "sub1", metadata = "sub1"} : tensor<1x5xf32>
    %13 = tensor.empty() : tensor<5x1xf32>
    %transposed_10 = linalg.transpose ins(%cst_4 : tensor<1x5xf32>) outs(%13 : tensor<5x1xf32>) permutation = [1, 0]
    %14 = tensor.empty() : tensor<1x1xf32>
    %15 = linalg.fill ins(%cst : f32) outs(%14 : tensor<1x1xf32>) -> tensor<1x1xf32>
    %16 = linalg.matmul ins(%12, %transposed_10 : tensor<1x5xf32>, tensor<5x1xf32>) outs(%15 : tensor<1x1xf32>) -> tensor<1x1xf32>
    debug.validate %16 {name = "layer3_matmul", metadata = "layer3_matmul"} : tensor<1x1xf32>
    %17 = linalg.generic {indexing_maps = [#map, #map1, #map], iterator_types = ["parallel", "parallel"]} ins(%16, %cst_3 : tensor<1x1xf32>, tensor<1xf32>) outs(%14 : tensor<1x1xf32>) {
    ^bb0(%in: f32, %in_12: f32, %out: f32):
      %27 = arith.addf %in, %in_12 : f32
      linalg.yield %27 : f32
    } -> tensor<1x1xf32>
    debug.validate %17 {name = "layer3_bias", metadata = "layer3_bias"} : tensor<1x1xf32>
    %18 = linalg.generic {indexing_maps = [#map, #map], iterator_types = ["parallel", "parallel"]} ins(%17 : tensor<1x1xf32>) outs(%14 : tensor<1x1xf32>) {
    ^bb0(%in: f32, %out: f32):
      %27 = math.tanh %in : f32
      linalg.yield %27 : f32
    } -> tensor<1x1xf32>
    debug.validate %18 {name = "layer3_tanh", metadata = "layer3_tanh"} : tensor<1x1xf32>
    %transposed_11 = linalg.transpose ins(%cst_2 : tensor<5x1xf32>) outs(%7 : tensor<1x5xf32>) permutation = [1, 0]
    %19 = linalg.matmul ins(%18, %transposed_11 : tensor<1x1xf32>, tensor<1x5xf32>) outs(%8 : tensor<1x5xf32>) -> tensor<1x5xf32>
    debug.validate %19 {name = "layer4_matmul", metadata = "layer4_matmul"} : tensor<1x5xf32>
    %20 = linalg.generic {indexing_maps = [#map, #map1, #map], iterator_types = ["parallel", "parallel"]} ins(%19, %cst_1 : tensor<1x5xf32>, tensor<5xf32>) outs(%7 : tensor<1x5xf32>) {
    ^bb0(%in: f32, %in_12: f32, %out: f32):
      %27 = arith.addf %in, %in_12 : f32
      linalg.yield %27 : f32
    } -> tensor<1x5xf32>
    debug.validate %20 {name = "layer4_bias", metadata = "layer4_bias"} : tensor<1x5xf32>
    %21 = linalg.generic {indexing_maps = [#map, #map], iterator_types = ["parallel", "parallel"]} ins(%20 : tensor<1x5xf32>) outs(%7 : tensor<1x5xf32>) {
    ^bb0(%in: f32, %out: f32):
      %27 = math.tanh %in : f32
      linalg.yield %27 : f32
    } -> tensor<1x5xf32>
    debug.validate %21 {name = "layer4_tanh", metadata = "layer4_tanh"} : tensor<1x5xf32>
    %22 = linalg.generic {indexing_maps = [#map, #map, #map], iterator_types = ["parallel", "parallel"]} ins(%12, %21 : tensor<1x5xf32>, tensor<1x5xf32>) outs(%7 : tensor<1x5xf32>) {
    ^bb0(%in: f32, %in_12: f32, %out: f32):
      %27 = arith.subf %in, %in_12 : f32
      linalg.yield %27 : f32
    } -> tensor<1x5xf32>
    debug.validate %22 {name = "sub2", metadata = "sub2"} : tensor<1x5xf32>
    %23 = linalg.generic {indexing_maps = [#map, #map], iterator_types = ["parallel", "parallel"]} ins(%22 : tensor<1x5xf32>) outs(%7 : tensor<1x5xf32>) {
    ^bb0(%in: f32, %out: f32):
      %27 = math.fpowi %in, %c2_i64 : f32, i64
      linalg.yield %27 : f32
    } -> tensor<1x5xf32>
    debug.validate %23 {name = "sq", metadata = "sq"} : tensor<1x5xf32>
    %24 = tensor.empty() : tensor<1xf32>
    %25 = linalg.fill ins(%cst : f32) outs(%24 : tensor<1xf32>) -> tensor<1xf32>
    %26 = linalg.generic {indexing_maps = [#map, #map2], iterator_types = ["parallel", "reduction"]} ins(%23 : tensor<1x5xf32>) outs(%25 : tensor<1xf32>) {
    ^bb0(%in: f32, %out: f32):
      %27 = arith.addf %in, %out : f32
      linalg.yield %27 : f32
    } -> tensor<1xf32>
    debug.validate %26 {name = "sse", metadata = "sse"} : tensor<1xf32>
    return %26, %21 : tensor<1xf32>, tensor<1x5xf32>
  }
}

{-#
  dialect_resources: {
    builtin: {
      torch_tensor_5_torch.float32_1: "0x04000000DF9C363E2B252FBE97BAC63C7E4612BEB81E333E",
      torch_tensor_5_1_torch.float32: "0x040000004E9D60BFD8D4583F146EDCBDF620393F324A58BF",
      torch_tensor_1_5_torch.float32: "0x040000003B46AFBED2D2A63EE27D2CBD5893903E1270A0BE",
      torch_tensor_5_torch.float32: "0x04000000192C13BF17844D3F941250401C92EA3E5766A83F",
      torch_tensor_5_4_torch.float32: "0x04000000F6CCC1BEB1F15E40EF7CBEBF9CE2803F3B92EE3F7E43513F8C1541C00F3739BEC0A84CC035912BBF3E86D9BFCE4206BF10DA74BF561251402A74503FC08614C00945EC3F523E893D51B33BBF93633DC0",
      torch_tensor_4_torch.float32: "0x04000000AD4F873FEA4625C0168A70401AA64640",
      torch_tensor_4_5_torch.float32: "0x04000000E8692DBECDC8BF3FB6CC60C07B2584BF9F9E9F3F3FBC3C40D8722A3EDC27B4BFD10E1B40AA1125BF7D8282BF7A3358C0986A29C06DA9AF3FBCB849BF3969E73FF6480E3E6BBCB8BF71270FC029D85BC0"
    }
  }
#-}
