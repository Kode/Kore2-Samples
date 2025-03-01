import Metal
import Darwin

let device = MTLCreateSystemDefaultDevice()
if device == nil {
	print("This device does not support Metal")
	exit(1)
}
print("This device does support Metal")
dump(device)
