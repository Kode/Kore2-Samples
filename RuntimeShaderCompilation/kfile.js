const project = new Project('ShaderTest');

// For Vulkan it can be important to add krafix
// before Kinc to prevent header-chaos
const krafix = await project.addProject('krafix');
krafix.useAsLibrary();

await project.addProject('../Kore');

project.addFile('Sources/**');
project.setDebugDir('Deployment');

if (Options.screenshot) {
	project.addDefine('SCREENSHOT');
	project.addDefine('KOPE_D3D12_FORCE_WARP');
}

project.flatten();

resolve(project);
