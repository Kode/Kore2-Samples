const fs = require('fs');
const path = require('path');

const samples = [
	'00_empty',
  '01_triangle',
  '02_matrix',
  '03_colored_cube',
  '04_textured_cube',
  '05_camera_controls',
  '06_render_targets',
  '07_multiple_render_targets',
  '08_float_render_targets',
  '09_depth_render_targets',
  '10_cubemap',
  '11_instanced_rendering',
  '12_set_render_target_depth',
  '13_generate_mipmaps',
  '14_set_mipmap',
  '15_deinterleaved_buffers',
  'shader',
  'texture',
  //'multiwindow',
  'computeshader',
  'texturearray',
  //'runtime_shader_compilation',
  'raytracing',
  'bindless'
];

const workflowsDir = path.join('workflows');

function writeWorkflow(workflow) {
  if (workflow.sys === 'FreeBSD') {
    writeFreeBSDWorkflow(workflow);
    return;
  }
  if (workflow.sys === 'Linux' && workflow.cpu === 'ARM') {
    writeLinuxArmWorkflow(workflow);
    return;
  }

  const java = workflow.java
?
`
    - uses: actions/setup-java@v3
      with:
        distribution: 'oracle'
        java-version: '17'
`
:
'';

  const steps = workflow.steps ?? '';
  const postfixSteps = workflow.postfixSteps ?? '';

  const workflowName = workflow.gfx ? (workflow.sys + ' (' + workflow.gfx + ')') : workflow.sys;
  let workflowText = `name: ${workflowName}

on:
  push:
    branches:
    - v3
  pull_request:
    branches:
    - v3

jobs:
  build:

    runs-on: ${workflow.runsOn}

    steps:
    - uses: actions/checkout@v4
${java}
${steps}
`;

if (workflow.sys === 'Windows' && workflow.gfx === 'Direct3D 12') {
    workflowText +=
`    - name: Install WARP 1.0.13
      run: nuget install Microsoft.Direct3D.WARP
`;
}

workflowText +=
`    - name: Get Submodules
      run: ./get_dlc
${postfixSteps}
`;

  for (let index = 0; index < samples.length; ++index) {
    if (!workflow.checked[index]) {
      continue;
    }

    const sample = samples[index];

    if (sample === 'RuntimeShaderCompilation') {
      if (!workflow.RuntimeShaderCompilation) {
        continue;
      }
    }

    if (workflow.noCompute && sample === 'ComputeShader') {
      continue;
    }

    if (workflow.noTexArray && sample === 'TextureArray') {
      continue;
    }

    const prefix = workflow.compilePrefix ?? '';
    const postfix = workflow.compilePostfix ?? '';
    const gfx = workflow.gfx ? ((workflow.gfx === 'WebGL') ? ' -g opengl' : ' -g ' + workflow.gfx.toLowerCase().replace(/ /g, '')) : '';
    const options = workflow.options ? ' ' + workflow.options : '';
    const sys = workflow.sys === 'macOS' ? 'osx' : (workflow.sys === 'UWP' ? 'windowsapp' : workflow.sys.toLowerCase());
    const vs = workflow.vs ? ' -v ' + workflow.vs : '';

    if (workflow.sys === 'Windows' && workflow.gfx === 'Direct3D 12') {
      workflowText +=
`    - name: Copy WARP
      working-directory: ${sample}
      run: echo F|xcopy D:\\a\\Kore-Samples\\Kore-Samples\\Microsoft.Direct3D.WARP.1.0.13\\build\\native\\bin\\x64\\d3d10warp.dll Deployment\\d3d10warp.dll
`;
    }

    workflowText +=
`    - name: Compile ${sample}
      working-directory: ${sample}
      run: ${prefix}../Kore/make ${sys}${vs}${gfx}${options} --debug`;

    if (workflow.canExecute) {
      workflowText += ' --option screenshot --run';
    }
    else {
      workflowText += ' --compile';
    }

    workflowText += postfix + '\n';

    if (workflow.env) {
      workflowText += workflow.env;
    }
 
    if (workflow.canExecute) {
      workflowText +=
`    - name: Check ${sample}
      working-directory: ${sample}
      run: magick compare -metric mae .\\reference.png .\\Deployment\\test.png difference.png
    - name: Upload  ${sample} failure image
      if: failure()
      uses: actions/upload-artifact@v4
      with:
        name: ${sample} image
        path: ${sample}/Deployment/test.png
`;
    }
  }

  const name = workflow.gfx ? (workflow.sys.toLowerCase() + '-' + workflow.gfx.toLowerCase().replace(/ /g, '')) : workflow.sys.toLowerCase();
  fs.writeFileSync(path.join(workflowsDir, name + '.yml'), workflowText, {encoding: 'utf8'});
}

const workflows = [
  /*{
    sys: 'Android',
    gfx: 'OpenGL',
    runsOn: 'ubuntu-latest',
    java: true
  },*/
  {
    sys: 'Android',
    gfx: 'Vulkan',
    runsOn: 'ubuntu-latest',
    java: true,
    canExecute: false,
    checked: [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0]
  },
  /*{
    sys: 'Emscripten',
    gfx: 'WebGL',
    runsOn: 'ubuntu-latest',
    steps: '',
    compilePrefix: '../emsdk/emsdk activate latest && source ../emsdk/emsdk_env.sh && ',
    compilePostfix: ' && cd build/Release && make',
    postfixSteps:
`    - name: Setup emscripten
      run: git clone https://github.com/emscripten-core/emsdk.git && cd emsdk && ./emsdk install latest
`,
    noCompute: true,
    noTexArray: true
  },
  {
    sys: 'Emscripten',
    gfx: 'WebGPU',
    runsOn: 'ubuntu-latest',
    steps: '',
    compilePrefix: '../emsdk/emsdk activate latest && source ../emsdk/emsdk_env.sh && ',
    compilePostfix: ' && cd build/Release && make',
    postfixSteps:
`    - name: Setup emscripten
      run: git clone https://github.com/emscripten-core/emsdk.git && cd emsdk && ./emsdk install latest
`
  },
  {
    sys: 'FreeBSD',
    gfx: 'OpenGL',
    runsOn: 'macos-12'
  },*/
  {
    sys: 'iOS',
    gfx: 'Metal',
    runsOn: 'macOS-latest',
    options: '--nosigning',
    canExecute: false,
    checked: [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0]
  },/*
  {
    sys: 'iOS',
    gfx: 'OpenGL',
    runsOn: 'macOS-latest',
    options: '--nosigning',
    noCompute: true
  },
  {
    sys: 'Linux',
    gfx: 'OpenGL',
    cpu: 'ARM'
  },
  {
    sys: 'Linux',
    gfx: 'OpenGL',
    runsOn: 'ubuntu-latest',
    steps:
`    - name: Apt Update
      run: sudo apt update
    - name: Apt Install
      run: sudo apt-get install libasound2-dev libxinerama-dev libxrandr-dev libgl1-mesa-dev libxi-dev libxcursor-dev libudev-dev libwayland-dev wayland-protocols libxkbcommon-dev ninja-build --yes --quiet
`,
    RuntimeShaderCompilation: true
  },
  {
    sys: 'Linux',
    gfx: 'Vulkan',
    runsOn: 'ubuntu-latest',
    steps:
`    - name: Get LunarG key
      run: wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc
    - name: Get LunarG apt sources
      run: sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.3.275-jammy.list https://packages.lunarg.com/vulkan/1.3.275/lunarg-vulkan-1.3.275-jammy.list
    - name: Apt Update
      run: sudo apt update
    - name: Apt Install
      run: sudo apt install libasound2-dev libxinerama-dev libxrandr-dev libgl1-mesa-dev libxi-dev libxcursor-dev libudev-dev vulkan-sdk libwayland-dev wayland-protocols libxkbcommon-dev ninja-build --yes --quiet
`
  },
  */{
    sys: 'macOS',
    gfx: 'Metal',
    runsOn: 'macOS-latest',
    canExecute: false,
    checked: [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0]
  },
  /*{
    sys: 'macOS',
    gfx: 'OpenGL',
    runsOn: 'macOS-latest'
  },
  {
    sys: 'UWP',
    runsOn: 'windows-latest',
    vs: 'vs2022'
  },
  {
    sys: 'Windows',
    gfx: 'Direct3D 9',
    runsOn: 'windows-latest',
    noCompute: true,
    noTexArray: true,
    vs: 'vs2022',
    postfixSteps:
`    - name: Install DirectX
      run: choco install -y directx --no-progress`
  },
  {
    sys: 'Windows',
    gfx: 'Direct3D 11',
    runsOn: 'windows-latest',
    RuntimeShaderCompilation: true,
    vs: 'vs2022'
  },*/
  {
    sys: 'Windows',
    gfx: 'Direct3D 12',
    runsOn: 'windows-latest',
    canExecute: true,
    vs: 'vs2022',
    checked: [1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0]
  },/*
  {
    sys: 'Windows',
    gfx: 'OpenGL',
    runsOn: 'windows-latest',
    vs: 'vs2022'
  },*/
  {
    sys: 'Windows',
    gfx: 'Vulkan',
    runsOn: 'windows-latest',
    canExecute: true,
    vs: 'vs2022',
    env:
`      env:
        VULKAN_SDK: C:\\VulkanSDK\\1.3.275.0
`,
    steps:
`    - name: Setup Vulkan
      run: |
          Invoke-WebRequest -Uri "https://sdk.lunarg.com/sdk/download/1.3.275.0/windows/VulkanSDK-1.3.275.0-Installer.exe" -OutFile VulkanSDK.exe
          $installer = Start-Process -FilePath VulkanSDK.exe -Wait -PassThru -ArgumentList @("--da", "--al", "-c", "in");
          $installer.WaitForExit();`,
    checked: [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
  }
];

for (const workflow of workflows) {
  writeWorkflow(workflow);
}
