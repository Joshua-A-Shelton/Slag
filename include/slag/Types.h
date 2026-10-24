#pragma once

// Small value types shared by several headers, so object headers don't include
// each other for one enum or struct: CompareOp (samplers, depth-stencil state),
// Viewport and Rect (command buffers), ClearValue (textures, render passes).
