const project = new Project('08_float_render_targets');

await project.addProject(findKore());

project.addFile('sources/**');
project.addKongDir('shaders');
project.setDebugDir('deployment');

if (Options.screenshot) {
	project.addDefine('SCREENSHOT');
	project.addDefine('KORE_D3D12_FORCE_WARP');
}

project.flatten();

resolve(project);
