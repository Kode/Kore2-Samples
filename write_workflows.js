const fs = require('fs');
const path = require('path');

const samples = [
	'Shader',
	'Texture',
	//'MultiWindow',
	'ComputeShader',
	'TextureArray',
  //'RuntimeShaderCompilation',
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
  '15_deinterleaved_buffers'
];

const workflowsDir = path.join('.github', 'workflows');

function writeFreeBSDWorkflow(workflow) {
  const steps = workflow.steps ?? '';
  const postfixSteps = workflow.postfixSteps ?? '';

  const workflowName = workflow.gfx ? (workflow.sys + ' (' + workflow.gfx + ')') : workflow.sys;
  let workflowText = `name: ${workflowName}

on:
  push:
    branches:
    - main
  pull_request:
    branches:
    - main

jobs:
  build:

    runs-on: ${workflow.runsOn}

    steps:
    - uses: actions/checkout@v3
${steps}
    - name: Get Submodules
      run: ./get_dlc
    - name: Get the FreeBSD-submodule
      run: git -C Kinc submodule update --init Tools/freebsd_x64
    - name: Compile in FreeBSD VM
      id: build
      uses: vmactions/freebsd-vm@v0
      with:
        usesh: true
        copyback: false
        mem: 2048
        prepare: pkg install -y bash alsa-lib libXinerama mesa-libs libXi xorg-vfbserver libXrandr libXi libXcursor evdev-proto libinotify ImageMagick7-nox11 libxkbcommon
        run: |
`;

  for (const sample of samples) {
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

    workflowText +=
`          echo " * Compile ${sample}"
          cd ${sample}
          ../Kinc/make ${sys}${vs}${gfx}${options} --compile${postfix}
          cd ..
`;
    if (workflow.env) {
      workflowText += workflow.env;
    }
  }

  const name = workflow.gfx ? (workflow.sys.toLowerCase() + '-' + workflow.gfx.toLowerCase().replace(/ /g, '')) : workflow.sys.toLowerCase();
  fs.writeFileSync(path.join(workflowsDir, name + '.yml'), workflowText, {encoding: 'utf8'});
}

function writeLinuxArmWorkflow(workflow) {
  const steps = workflow.steps ?? '';
  const postfixSteps = workflow.postfixSteps ?? '';

  const workflowName = workflow.gfx ? (workflow.sys + ' (' + workflow.gfx + ')') : workflow.sys;
  let workflowText = `name: Linux on ARM (OpenGL)

on:
  push:
    branches:
    - main
  pull_request:
    branches:
    - main

jobs:
  build:
    runs-on: ubuntu-latest
    name: Build on \${{ matrix.distro }} \${{ matrix.arch }}

    # Run steps for both armv6 and aarch64
    strategy:
      matrix:
        include:
          - arch: aarch64
            distro: ubuntu20.04
          - arch: armv7
            distro: ubuntu20.04

    steps:
      - uses: actions/checkout@v3
      - uses: uraimo/run-on-arch-action@v2.0.9
        name: Run Tests in \${{ matrix.distro }} \${{ matrix.arch }}
        id: build
        with:
          arch: \${{ matrix.arch }}
          distro: \${{ matrix.distro }}

          # Not required, but speeds up builds
          githubToken: \${{ github.token }}

          # The shell to run commands with in the container
          shell: /bin/bash

          # Install some dependencies in the container. This speeds up builds if
          # you are also using githubToken. Any dependencies installed here will
          # be part of the container image that gets cached, so subsequent
          # builds don't have to re-install them. The image layer is cached
          # publicly in your project's package repository, so it is vital that
          # no secrets are present in the container state or logs.
          install: |
              apt-get update -y -q
              apt-get upgrade -y -q
              apt-get install -y -q libasound2-dev libxinerama-dev libxrandr-dev libgl1-mesa-dev libxi-dev libxcursor-dev libudev-dev git build-essential imagemagick xvfb libwayland-dev wayland-protocols libxkbcommon-dev ninja-build

          # Produce a binary artifact and place it in the mounted volume
          run: |
            echo " * Make Git happy"
            git config --global --add safe.directory /home/runner/work/KincKong-Samples/KincKong-Samples
            git config --global --add safe.directory /home/runner/work/KincKong-Samples/KincKong-Samples/Kinc
            git config --global --add safe.directory /home/runner/work/KincKong-Samples/KincKong-Samples/Kinc/Tools/linux_arm
            git config --global --add safe.directory /home/runner/work/KincKong-Samples/KincKong-Samples/Kinc/Tools/linux_arm64
            echo " * Get Submodules"
            ./get_dlc
`;

  for (const sample of samples) {
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

    workflowText +=
`            echo " * Compile ${sample}"
            cd ${sample}
            ../Kinc/make -g opengl --compile
            cd ..
`;
    if (workflow.env) {
      workflowText += workflow.env;
    }
  }

  fs.writeFileSync(path.join(workflowsDir, 'linux-arm-opengl.yml'), workflowText, {encoding: 'utf8'});
}

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
    - main
  pull_request:
    branches:
    - main

jobs:
  build:

    runs-on: ${workflow.runsOn}

    steps:
    - uses: actions/checkout@v4
${java}
${steps}
    - name: Get Submodules
      run: ./get_dlc
${postfixSteps}
`;

  for (const sample of samples) {
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

    workflowText +=
`    - name: Compile ${sample}
      working-directory: ${sample}
      run: ${prefix}../Kinc/make ${sys}${vs}${gfx}${options} --option screenshot --debug --run${postfix}
    - name: Check ${sample}
      working-directory: ${sample}
      run: magick compare -metric mae .\\reference.png .\\Deployment\\test.png difference.png
`;
    if (workflow.env) {
      workflowText += workflow.env;
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
  },
  {
    sys: 'Android',
    gfx: 'Vulkan',
    runsOn: 'ubuntu-latest',
    java: true
  },
  {
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
  },
  {
    sys: 'iOS',
    gfx: 'Metal',
    runsOn: 'macOS-latest',
    options: '--nosigning'
  },
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
  {
    sys: 'macOS',
    gfx: 'Metal',
    runsOn: 'macOS-latest',
    RuntimeShaderCompilation: true
  },
  {
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
    vs: 'vs2022'
  }/*,
  {
    sys: 'Windows',
    gfx: 'OpenGL',
    runsOn: 'windows-latest',
    vs: 'vs2022'
  },
  {
    sys: 'Windows',
    gfx: 'Vulkan',
    runsOn: 'windows-latest',
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
          $installer.WaitForExit();`
  }*/
];

for (const workflow of workflows) {
  writeWorkflow(workflow);
}
