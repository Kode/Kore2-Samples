const project = new Project('ColoredCube');

await project.addProject('../Kore', {kong: true, kope: true});

project.addFile('Sources/**');
project.addKongDir('Shaders');
project.setDebugDir('Deployment');

if (Options.screenshot) {
	project.addDefine('SCREENSHOT');
	project.addDefine('KOPE_D3D12_FORCE_WARP');
}

project.flatten();

resolve(project);
