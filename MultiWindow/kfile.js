const project = new Project('MultiWindow');

await project.addProject(findKore());

project.addFile('Sources/**');
project.addFile('Shaders/**');
project.setDebugDir('Deployment');

project.flatten();

resolve(project);
