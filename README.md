# PBR Deferred Renderer

A C+opengl rendering project where I implement various computer graphics techniques.

It currently includes an HDR deferred render pipeline, PBR lighting, shadow mapping, parallax mapping, tonemapping, basic particle emitters (forward rendered).

The material system uses a roughness/metallic workflow and supports any/all of the following textures:
* Roughness
* Metalness
* Normal
* Emissive
* AO
* Height

There is also a small tool to stitch cubemaps and bake irradiance maps (support for prefiltered mimapped radiance maps is NYI).

# Screenshots
![Screenshot1](https://raw.githubusercontent.com/tullrich/deferred/master/screenshots/ShadowMaps.png)

![Screenshot2](https://raw.githubusercontent.com/tullrich/deferred/master/screenshots/PurpleBuddha.png)

![Screenshot3](https://raw.githubusercontent.com/tullrich/deferred/master/screenshots/ParallaxMapping.png)

# References
* Material textures come from [freepbr.com](https://freepbr.com/)
* Environment maps come from [humus.name](http://www.humus.name/index.php?page=Textures)
* Lots of implementation help from [learnopengl.com](https://learnopengl.com/PBR/Theory)
