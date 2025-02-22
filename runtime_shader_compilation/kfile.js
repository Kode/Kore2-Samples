const project = new Project('RuntimeShaderCompilation');

// For Vulkan it can be important to add krafix
// before Kore to prevent header-chaos
const krafix = await project.addProject('krafix');
krafix.useAsLibrary();

await project.addProject(findKore());

project.addFile('sources/**');
project.setDebugDir('deployment');

if (Options.screenshot) {
	project.addDefine('SCREENSHOT');
	project.addDefine('KOPE_D3D12_FORCE_WARP');
}

project.flatten();

resolve(project);
