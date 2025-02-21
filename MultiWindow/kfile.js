const project = new Project('MultiWindow');

await project.addProject('../Kore');

project.addFile('Sources/**');
project.addFile('Shaders/**');
project.setDebugDir('Deployment');

if (Options.screenshot) {
	project.addDefine('SCREENSHOT');
	project.addDefine('KOPE_D3D12_FORCE_WARP');
}

project.flatten();

resolve(project);
