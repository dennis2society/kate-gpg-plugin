# Kate GPG Plugin

This plugin allows to GPG decrypt and encrypt text files 
using the [GPGMe library](https://gnupg.org/software/gpgme/index.html) 
in KDE's text editor KATE.

![image info](./kate_gpg_plugin_screenshot.jpg)

## Features
+ Plugin shows all available GPG keys with basic name filtering
  (auto-selects the most recently created key)
+ Manual selection of key used for encryption
+ Symmetric encryption possible

## Prerequisites
+ Qt development libraries are installed
+ A CMAKE & C++ build environment is installed
+ C/C++ bindings for GPGMe are installed
+ At least one public+private GPG key pair (if you want to encrypt to yourself)

## Caution!
While this plugin makes it easy to decrypt+encrypt text, it also makes it easy to
mess things up! You could accidentally encrypt a file, e.g. with a key 
that is not yours, which then you wouldn't be able to decrypt. Or you could accidentally
save a currently decrypted file as plain text, leaving it unecrypted.

+ Use with care!
+ Think twice before pressing Ctrl+S!
+ Ctrl+Z works after encryption and saving.

## Build Instructions
This plugin was developed and built on a recent Manjaro Linux running KDE Plasma. I have
tested the build in a fresh (K)Ubuntu 22.04.3 LTS VM and had to install at least these
packages manually:
<ul>
  <li>git</li>
  <li>cmake</li>
  <li>extra-cmake-modules</li>
  <li>cmake-extras</li>
  <li>g++</li>
  <li>kate</li>
  <li>libgpgmepp-dev (this should also install qmake and quite a few Qt dev libs)</li>
  <li>libgcrypt20-dev</li>
  <li>libgpg-error-dev</li>
  <li>libecm1-dev</li>
</ul>
This line should do it for most recent Ubuntu based distributions:

<code>sudo apt install git cmake extra-cmake-modules cmake-extras g++ kate libgpgmepp-dev libgcrypt20-dev libgpg-error-dev libecm1-dev</code>

### Build ###
<ul>
  <li>Clone the git repository</li>
  <li>Run CMake in the cloned folder (if it exists delete the <code>build/</code> folder first just to be sure):</li>
  <li>
    <code>
      cmake -B build/ -D CMAKE_BUILD_TYPE=Release
    </code>
  <li>
  <li>
    <code>cmake --build build/</code>
  </li>
  <li>
    Copy the plugin to the Kate plugin path. This requires sudo.<br />
    In my Ubuntu VM symlinking the plugin did not work. I had to copy the binry...
    Hint: You can use the CMAKE_INSTALL_PREFIX and then source the build/prefix.sh to add
    a custom non-root Kate plugin path:  [See here](https://develop.kde.org/docs/apps/kate/plugin/)<br />
    I have tried hard to get this working, but the plugin path on Ubuntu was always broken... :/
    <ul>
      <li>
        <b>Manjaro:</b><br />
        <code>sudo ln -s build/kate_gpg_plugin.so /usr/lib/qt/plugins/ktexteditor/<code>
      </li>
      <li>
        <b>(K)Ubuntu:</b><br />
        <code>sudo cp build/kate_gpg_plugin.so /usr/lib/x86_64-linux-gnu/qt5/plugins/ktexteditor/</code>
      </li>
    </ul>
  </li>
  <li>Run kate (from the current terminal prompt)</li>
  <li>Enable the GPG plugin in Kate -> Settings -> Preferences -> Configure Kate -> Plugins
    A new vertical button should appear in the left sidebar.
  </li>
</ul>
## TODO

+ Save/load current UI selections to/from disk
+ Fix that !#$%?X§ jumping UI bug
+ Automatically retrieve key fingerprint/ID and mail address 
  from encrypted file to set as selected "To:" key and mail address
+ Attach to KATE's "Open File" dialog to suggest automatic 
  decryption when a .gpg/.pgp/.asc file is opened
* Attach to KATE's Save/Save As dialog to strongly suggest to re-encrypt 
  a currently opened GPG file (to avoid saving it as unencrypted).
* Sign and verify documents
* Add support for subkeys

## Limitations

+ At the moment there exists an annoying UI bug where the plugin settings box jumps down when 
  changing the settings/document width. Setting the Kate window to fullscreen fixes this until 
  next resize.
+ No support for subkeys yet
+ Currently only the default email address for a key fingerprint will be used for encryption
+ Password prompts are handled by GPG(Me) and may look ugly. Won't touch this!


&copy; 2023, Dennis Lübke, kate-gpg-plugin (at) dennis2society.de
