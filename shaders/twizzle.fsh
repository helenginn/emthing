#version 330 core

varying vec4 vPos;
varying vec2 vTex;

uniform sampler2D pic_tex;
uniform sampler2D depth_tex;

uniform int horizontal;
uniform float threshold;
uniform float other;

vec4 make_heat(vec4 amp)
{
	if (amp.r > 2)
	{
		amp.b = amp.r - 2;
		amp.g = 1;
		amp.r = 1;
	}

	if (amp.r > 1)
	{
		amp.g = amp.r - 1;
		amp.r = 1;
	}

	return amp;
}

vec4 imageToNormal()
{
	vec4 r = texture(pic_tex, vTex);
	r.rgb -= other;
	r.rgb /= threshold;
	r.rgb = r.rgb / 2 + 0.5;
	r = make_heat(r);
	return r;
}

void main()
{
	ivec2 size = textureSize(pic_tex, 0);

	vec4 amp = vec4(0, 0, 0, 1);
	gl_FragColor = amp;

	if (horizontal == 1)
	{
		gl_FragColor = imageToNormal();
		return;
	}

	int radmax = 24;
	float rad = float(radmax) / float(size.y) * 2;

	int miny = size.y / 2 - radmax;
	int maxy = size.y / 2 + radmax;
	int minx = size.x / 2 - 2;
	int maxx = size.x / 2 + 2;

	float angle = radians(360. * vTex[0]);
	mat2 rot = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));

	for (int y = (miny); y < (maxy); y+=1)
	{
		for (int x = (minx); x < (maxx); x+=1)
		{
			vec2 pix;

			pix.x = float(x) / float(size.x);
			pix.y = float(y) / float(size.y);

			pix = rot * pix;

			amp.r += texture(pic_tex, pix).r;
		}
	}

	gl_FragColor = amp;
}


