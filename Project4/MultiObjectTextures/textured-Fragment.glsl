#version 150 core

struct Light {
    vec3 position;
    vec3 direction;
    float cutOff;
};

in vec3 Color;
in vec3 vertNormal;
in vec3 pos;     
in vec2 texcoord;

out vec4 outColor;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform int texID;
uniform Light light;

uniform float ambient;

void main() {
    vec3 color;
    if (texID == -1)
        color = Color;
    else if (texID == 0)
        color = texture(tex0, texcoord).rgb;
    else if (texID == 1)
        color = texture(tex1, texcoord).rgb;
    else if (texID == 2)
        color = texture(tex2, texcoord).rgb;
    else {
        outColor = vec4(1,0,0,1);
        return;
    }
    vec3 normal = normalize(vertNormal);
    vec3 lightDir = normalize(light.position - pos);
    vec3 viewDir = normalize(-pos);
    float theta = dot(lightDir, normalize(-light.direction));

    if (theta > light.cutOff) {
        //make flashlight look less rough
        float epsilon = 0.05; 
        float intensity = clamp((theta - light.cutOff)/epsilon, 0.0, 1.0);

        //lighting for flashlight
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * color;
        vec3 reflect = reflect(-lightDir, normal);
        float spec = max(dot(viewDir, reflect), 0.0);
        vec3 specC = 0.2*spec*vec3(1.0);
        vec3 ambC = color * ambient;
        vec3 oColor = ambC + intensity * (diffuse + specC);
        outColor = vec4(oColor, 1.0);
    } else { //outside the flashlight cone
        outColor = vec4(color * ambient, 1.0);
    }
}
