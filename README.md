# PBR Deferred Renderer

This is an SDL+OpenGL rendering project written in C that implements various computer graphics techniques I find interesting.

It currently includes an HDR deferred render pipeline, PBR lighting, IBL, shadow mapping, parallax mapping, tonemapping, basic particle emitters (forward rendered).

The material system uses a roughness/metallic workflow and supports any/all of the following textures:
* Roughness
* Metalness
* Normal
* Emissive
* AO
* Height

There is also a small tool to stitch cubemaps and bake irradiance maps (support for generating prefiltered radiance maps is NYI).

# Screenshots
![Screenshot1](https://raw.githubusercontent.com/tullrich/deferred/master/screenshots/GoldBunnyHDR.png)

![Screenshot2](https://raw.githubusercontent.com/tullrich/deferred/master/screenshots/TileBunnyHDR.png)

![Screenshot3](https://raw.githubusercontent.com/tullrich/deferred/master/screenshots/ShadowMaps.png)

![Screenshot4](https://raw.githubusercontent.com/tullrich/deferred/master/screenshots/PurpleBuddha.png)

![Screenshot5](https://raw.githubusercontent.com/tullrich/deferred/master/screenshots/ParallaxMapping.png)

![Screenshot6](https://raw.githubusercontent.com/tullrich/deferred/master/screenshots/TerracottaSphere.png)

# References
* Material textures come from [freepbr.com](https://freepbr.com/)
* Environment maps come from [humus.name](http://www.humus.name/index.php?page=Textures)
* Lots of implementation help from [learnopengl.com](https://learnopengl.com/PBR/Theory)
* [cmft](https://github.com/dariomanesku/cmft/tree/master/src/cmft) was used to generate the prefiltered radiance maps
* The GUI library used is the awesome [dear-imgui](https://github.com/ocornut/imgui)
