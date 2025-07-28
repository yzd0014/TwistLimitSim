--[[
	This file lists every asset that must be built by the AssetBuildSystem
]]

return
{
	shaders =
	{
		{ path = "Shaders/Vertex/vertexInputLayout.shader", arguments = { "vertex" } },
	},
	meshes = {
		"Meshes/cube.mesh",
		"Meshes/plane.mesh",
		"Meshes/ceiling.mesh",
		"Meshes/gun.mesh",
		"Meshes/bullet.mesh",
		"Meshes/outline.mesh",
		"Meshes/front_back_wall.mesh",
		"Meshes/missile.mesh",
		"Meshes/boss.mesh",
		"Meshes/1.mesh",
		"Meshes/2.mesh",
		"Meshes/3.mesh",
		"Meshes/4.mesh",
		"Meshes/5.mesh",
		"Meshes/6.mesh",
		"Meshes/7.mesh",
		"Meshes/8.mesh",
		"Meshes/9.mesh",
		"Meshes/0.mesh",
		"Meshes/square_plane.mesh",
		"Meshes/square_plane(8x8).mesh",
		"Meshes/cloth10x10.mesh",
		"Meshes/cloth2x2.mesh",
		"Meshes/tri.mesh",
		"Meshes/fem_cube.mesh",
		"Meshes/fem_cube_5.mesh",
		"Meshes/sphere.mesh",
		"Meshes/sphere4.mesh",
		"Meshes/pendulum1.mesh",
		"Meshes/pendulum2.mesh",
		"Meshes/wheel.mesh",
		"Meshes/box.mesh",
		"Meshes/shell.mesh",
		"Meshes/material_point.mesh",
        "Meshes/capsule.mesh",
        "Meshes/arrow.mesh",
	},
	effects = {
		"Effects/default.effect",
		"Effects/red.effect",
		"Effects/blue.effect",
		"Effects/white_front_light.effect",
		"Effects/white_back_light.effect",
		"Effects/text_r.effect",
		"Effects/text_l.effect",
	},
}
