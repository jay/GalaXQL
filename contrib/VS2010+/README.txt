This is a Visual Studio 2010+ solution that can be used to build GalaXQL. It
depends on wxWidgets 3 and uses a wxWidgets template available from NuGet, a
package manager for the Microsoft platform. NuGet is included by default with
most versions of Visual Studio 2012+. If you have Visual Studio 2010 or NuGet
was not included, then download NuGet using Visual Studio's Extension Manager:

Tools > Extension Manager > Online Gallery > NuGet Package Manager

When you open or attempt to build the solution for the first time NuGet should
attempt package restoration of the wxWidgets template. "NuGet Package
wxWidgetsTemplate XX is restored." But if that fails you may need to reinstall:

http://wiki.wxwidgets.org/Microsoft_Visual_C++_NuGet
Tools > NuGet Package Manager > Manage NuGet Packages > Installed Packages

After the template is installed there will be a new entry on the property pages
'wxWidgets' that you can use to set your wxWidgets configuration:

Right-click on GalaXQL project > Common/Configuration Properties > wxWidgets

***
You may have to exit Visual Studio and reopen the solution after the package is
restored to see wxWidgets in the property pages. Until you have set your
wxWidgets configuration you will not be able to build GalaXQL. Download
wxWidgets source and/or binaries at http://www.wxwidgets.org
***

Note the configuration option "Build wxWidgets library? No". While it might
seem intuitive to change that option to 'Yes' it makes more sense to leave it
as 'No' which will link to an already built version. For example you build
wxWidgets separately using its solution.

Note the configuration option "wxWidgets path".  It expects your wxWidgets
"root location", or in other words the location that contains your wxWidgets
include and lib directories, at the least.
