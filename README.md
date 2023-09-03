# Kate GPG Plugin

This plugin allows to GPG decrypt and encrypt text files 
using the [GPGMe library](https://gnupg.org/software/gpgme/index.html) 
in KDE's text editor KATE.

![A screenshot of the GPG plugin settings](./kate_gpg_plugin_screenshot.jpg)

## Features
+ Plugin shows all available GPG keys with basic name filtering
  (auto-selects the most recently created key)
+ Manual selection of key used for encryption
+ Symmetric encryption possible

## Prerequisites
+ A CMake & C++ build environment is installed
+ Qt development libraries are installed
+ C/C++ bindings for GPGMEpp are installed
+ At least one public+private GPG key pair (if you only want to encrypt to yourself)

## Caution!
While this plugin makes it easy to decrypt+encrypt text, it also makes it easy to
mess things up. You could accidentally encrypt a file, e.g. with a key
that is not yours, which then you wouldn't be able to decrypt. Or you could accidentally
save a currently decrypted file as plain text, leaving it unecrypted.

+ Use with care!
+ Think twice before pressing Ctrl+S!
+ Ctrl+Z works after encryption and saving.

## Build Instructions

### Dependencies
This plugin was developed and built on Manjaro Linux running KDE Plasma. I have
tested the build in a fresh non-KDE Ubuntu 22.04.3 LTS VM and had to install at least these
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

This line should do it for recent Ubuntu based distributions:<br />
<code>sudo apt install git cmake extra-cmake-modules cmake-extras g++ kate libgpgmepp-dev libgcrypt20-dev libgpg-error-dev libecm1-dev</code>

### Build
<ul>
  <li>Clone the git repository</li>
  <li>Run CMake in the cloned folder:</li>
  <ul>
    <li>
      <code>cmake -B build/ -D CMAKE_BUILD_TYPE=Release</code>
    </li>
  </ul>
  <li>
    <ul>
      <li>
        <code>cmake --build build/</code>
      </li>
    </ul>
  </li>
  <li>
    Install the plugin to the Kate plugin path. This requires sudo!<br />
    <ul>
      <li>
        <b>Recommended: </b><br /><code>sudo cmake --install build/</code><br />
        (I had cases when this was unreliable... If this does not work see manual installation below)<br />
      </li>
      <li>
        <b>Manual installation in Manjaro:</b><br />
        <code>sudo cp build/kate_gpg_plugin.so /usr/lib/qt/plugins/ktexteditor/</code><br />
        or if you prefer a symlink:<br />
        <code>sudo ln -s build/kate_gpg_plugin.so /usr/lib/qt/plugins/ktexteditor/</code><br />
      </li>
      <li>
        <b>Manual installation in Ubuntu:</b><br /><code>sudo cp build/kate_gpg_plugin.so /usr/lib/x86_64-linux-gnu/qt5/plugins/ktexteditor/</code><br />
        (In my Ubuntu VM symlinking the plugin did not work. Plugin doesn't show up in Kate unless copied...)<br />
      </li>
    </ul>
  </li>
  <li>Run kate (from the current terminal prompt)</li>
  <li>Enable the "<b>GPG Plugin</b>" in Kate &rarr; Settings &rarr; Configure Kate &rarr; Plugins.<br />
    A new vertical button should appear in the left sidebar.<br />
  </li>
</ul>

## Limitations

+ Currently only the default email address for a key fingerprint will be used for encryption
+ No support for subkeys yet
+ Password prompts are handled by GPG(Me) and may look ugly. Won't touch this!

## TODO ##

+ Automatically retrieve key fingerprint/ID and mail address
  from encrypted file to set as selected "To:" key and mail address
+ Attach to KATE's "Open File" dialog to suggest automatic
  decryption when a .gpg/.pgp/.asc file is opened
* Attach to KATE's Save/Save As dialog to strongly suggest to re-encrypt
  a currently opened GPG file (to avoid saving it as unencrypted).
* Sign and verify documents
* Add support for subkeys

&copy; 2023, Dennis Lübke, kate-gpg-plugin (at) dennis2society.de
