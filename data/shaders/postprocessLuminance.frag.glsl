#extension GL_ARB_texture_rectangle : enable
uniform sampler2DRect fboTex;
// downscale stage of making the bloom texture

void main(void)
{
	const float delta = 0.001;
	vec3 col = vec3(texture2DRect(fboTex, gl_TexCoord[0].st));
	gl_FragColor = vec4(log(delta + dot(col, vec3(0.299,0.587,0.114))));
}
