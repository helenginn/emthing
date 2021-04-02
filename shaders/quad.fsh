#version 330 core

varying vec4 vPos;
varying vec2 vTex;

uniform sampler2D pic_tex;
uniform sampler2D depth_tex;

uniform int horizontal;
uniform float threshold;
uniform float other;

float contribution_for_pixel(vec2 pix, vec2 r1, float len2, 
							 float rad, float angle)
{
/* positive density is less than 0.5 */
	float orig = texture(pic_tex, pix).r - 0.5;

	if (orig > 0)
	{
//		return -1.;
	}

	float first = texture(pic_tex, r1).r - 0.5;

	if (first > 0)
	{
//		return -1.;
	}

	float sum = 0;
	int count = 0;

	vec2 radius2 = vec2(sin(radians(angle + threshold)), 
				       -cos(radians(angle + threshold)));
	radius2 *= len2;
	vec2 r2 = pix + radius2;
	if (length(r2 - vec2(0.5, 0.5)) > rad) return 0.;

	float second = texture(pic_tex, r2).r - 0.5;

	if (second > 0)
	{
//		return -1.;
	}

	if (!((first > 0 && second > 0 && orig > 0) ||
		  (first < 0 && second < 0 && orig < 0)))
	{
		return -1.;
	}

	sum += -(orig * first * second);

	return sum;
}

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

vec4 imageToNormal(float thresh)
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
	size = ivec2(128, 128);

	vec4 amp = vec4(0, 0, 0, 1);
	gl_FragColor = amp;
	float thresh = 0.33;

	if (horizontal == 1)
	{
		gl_FragColor = imageToNormal(thresh);
		return;
	}

	if (vTex.x < 0.05 || vTex.y < 0.05 || 
			length(vTex) < 0.1 || length(vTex) > thresh || 
			vTex.y > vTex.x)
	{
		gl_FragColor = amp;
		return;
	}

	int radmax = 14;
	float rad = float(radmax) / float(size.y) * 2;

	int miny = size.y / 2 - radmax;
	int maxy = size.y / 2 + radmax;
	int minx = size.x / 2 - radmax;
	int maxx = size.x / 2 + radmax;

	for (float angle = 0; angle < 359; angle += 30)
	{
		vec2 radius1 = vec2(sin(radians(angle)), -cos(radians(angle)));
		radius1 *= vTex.x;

		for (int y = (miny); y < (maxy); y+=1)
		{
			for (int x = (minx); x < (maxx); x+=1)
			{
				vec2 pix;

				pix.x = float(x) / float(size.x);
				pix.y = float(y) / float(size.y);
			
				if (length(pix - vec2(0.5, 0.5)) > rad) continue;
				vec2 r1 = pix + radius1;

				if (length(r1 - vec2(0.5, 0.5)) > rad) continue;

				float result = contribution_for_pixel(pix, r1, vTex.y, rad,
												      angle);

				if (result > 0) 
				{
					amp.r += result;
				}
			}
		}
	}

	vec3 base = texture(depth_tex, vTex).rgb;
	gl_FragColor = amp + vec4(base, 0.);
}


