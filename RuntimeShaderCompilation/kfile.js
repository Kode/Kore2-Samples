const project = new Project('ShaderTest');

// For Vulkan it can be important to add krafix
// before Kore to prevent header-chaos
const krafix = await project.addProject('krafix');
krafix.useAsLibrary();

await project.addProject(findKore());

project.addFile('Sources/**');
project.setDebugDir('Deployment');

project.flatten();

resolve(project);
