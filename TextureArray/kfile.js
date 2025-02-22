const project = new Project('TextureArray');

await project.addProject(findKore());

project.addFile('Sources/**');
project.addFile('Shaders/**');
project.setDebugDir('Deployment');

project.flatten();

resolve(project);
