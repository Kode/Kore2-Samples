const project = new Project('TextureTest');

await project.addProject('../Kinc', {kong: true, kope: true});

project.addFile('Sources/**');
project.addKongDir('Shaders');
project.setDebugDir('Deployment');

project.flatten();

resolve(project);
