const project = new Project('Compute');

await project.addProject('../Kinc', {kong: true, kope: true});

project.addFile('Sources/**');
project.addFile('Shaders/**');
project.setDebugDir('Deployment');

project.flatten();

resolve(project);
