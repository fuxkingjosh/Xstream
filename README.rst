XStream
=======

XStream is a personal fork of OBS Studio for video recording and live streaming.
It keeps the OBS Studio plugin API, libraries, and file formats, and it is
distributed under the GNU General Public License v2 (or any later version).
Based on OBS Studio by the OBS Project.

Browser sources and OBS plugins
-------------------------------

Browser sources (the globe icon: GIF overlays, webcam frames, name cards)
come from the obs-browser module and Chromium Embedded Framework. They are
part of a Windows build. A build configured without that module still opens
a scene collection, but those sources show red names and a generic icon and
do not render. CI and a normal Windows configure both enable the module.
Rebuild, or download the Windows zip from the workflow run, and run
``bin/64bit/obs64.exe``. The window title says XStream. Replacing only the
source tree does not update a build that was compiled without obs-browser.

XStream still stores its config in the ``obs-studio`` folder and loads
third-party plugins from the same places as OBS Studio:

- Recommended: ``C:\ProgramData\obs-studio\plugins\<name>\<name>.dll`` with a
  ``data`` folder beside the DLL.
- Legacy, next to a normal OBS Studio install:
  ``C:\Program Files\obs-studio\obs-plugins\64bit\<name>.dll`` and
  ``C:\Program Files\obs-studio\data\obs-plugins\<name>\``.
  Plugins such as Aitum Multistream that the OBS installer put there are
  loaded as well. XStream does not load OBS's own ``core`` modules from that
  install; browser support has to be the copy shipped with XStream, because
  that copy is built against this tree and includes CEF.

If a plugin does not show up, copy it into the ProgramData layout above or
set ``OBS_PLUGINS_PATH`` to a directory of ``<name>\<name>.dll`` folders.
Portable mode skips ProgramData and still checks the OBS Studio install.

OBS Studio <https://obsproject.com>
===================================

.. image:: https://github.com/obsproject/obs-studio/actions/workflows/push.yaml/badge.svg?branch=master
   :alt: OBS Studio Build Status - GitHub Actions
   :target: https://github.com/obsproject/obs-studio/actions/workflows/push.yaml?query=branch%3Amaster

.. image:: https://badges.crowdin.net/obs-studio/localized.svg
   :alt: OBS Studio Translation Project Progress
   :target: https://crowdin.com/project/obs-studio

.. image:: https://img.shields.io/discord/348973006581923840.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2
   :alt: OBS Studio Discord Server
   :target: https://obsproject.com/discord

What is OBS Studio?
-------------------

OBS Studio is software designed for capturing, compositing, encoding,
recording, and streaming video content, efficiently.

It's distributed under the GNU General Public License v2 (or any later
version) - see the accompanying COPYING file for more details.

Quick Links
-----------

- Website: https://obsproject.com

- Help/Documentation/Guides: https://github.com/obsproject/obs-studio/wiki

- Forums: https://obsproject.com/forum/

- Build Instructions: https://github.com/obsproject/obs-studio/wiki/Install-Instructions

- Developer/API Documentation: https://obsproject.com/docs

- Donating/backing/sponsoring: https://obsproject.com/contribute

- Bug Tracker: https://github.com/obsproject/obs-studio/issues

Contributing
------------

- If you would like to help fund or sponsor the project, you can do so
  via `Patreon <https://www.patreon.com/obsproject>`_, `OpenCollective
  <https://opencollective.com/obsproject>`_, or `PayPal
  <https://www.paypal.me/obsproject>`_.  See our `contribute page
  <https://obsproject.com/contribute>`_ for more information.

- If you wish to contribute code to the project, please make sure to
  read the coding and commit guidelines:
  https://github.com/obsproject/obs-studio/blob/master/CONTRIBUTING.md
  
- Code for the project follows the code style guidelines, located
  here: https://github.com/obsproject/obs-studio/blob/master/CODESTYLE.md

- Developer/API documentation can be found here:
  https://obsproject.com/docs

- If you wish to contribute translations, do not submit pull requests.
  Instead, please use Crowdin.  For more information read this page:
  https://obsproject.com/wiki/How-To-Contribute-Translations-For-OBS

- Contributors to OBS Studio and related repositories are expected to
  follow our Code of Conduct, which can be read here:
  https://github.com/obsproject/obs-studio/blob/master/COC.rst

- Other ways to contribute are by helping people out with support on
  our forums or in our community chat.  Please limit support to topics
  you fully understand -- bad advice is worse than no advice.  When it
  comes to something that you don't fully know or understand, please
  defer to the official help or official channels.


SAST Tools
----------

`PVS-Studio <https://pvs-studio.com/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source>`_ - static analyzer for C, C++, C#, and Java code.
