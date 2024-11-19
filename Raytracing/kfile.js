const project = new Project('Raytracing');

await project.addProject('../Kinc', {kong: true, kope: true});

project.addFile('Sources/**');
project.addKongDir('Shaders');
project.setDebugDir('Deployment');

/*project.addIncludeDir('../nvapi');
project.addLib('../nvapi/amd64/nvapi64');
project.addDefine('KOPE_NVAPI');*/

if (Options.screenshot) {
	project.addDefine('SCREENSHOT');
	project.addDefine('KOPE_D3D12_FORCE_WARP');
}

project.flatten();

resolve(project);
