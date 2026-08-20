#version 450 core
in vec2 vNDC;
out vec4 fragColor;

uniform mat4 uInvView;
uniform mat4 uInvProj;
uniform vec3 uCameraPos;
uniform sampler3D uVolume;
uniform float uStepSize;
uniform int uMaxSteps;
uniform vec3 uVolumeHalfSize;
uniform sampler1D uTransferFunction;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform int uRenderMode;
uniform float uIsoValue;
uniform vec3 uClipNormal;
uniform float uClipOffset;
uniform bool uClipEnabled;

void main() {
    vec4 clipPos = vec4(vNDC.x, vNDC.y, -1.0, 1.0);
    vec4 viewPos = uInvProj * clipPos;
    viewPos = vec4(viewPos.xy / viewPos.w, -1.0, 0.0);
    vec4 worldDir = uInvView * viewPos;
    vec3 rayDir = normalize(worldDir.xyz);

    vec3 tMin = (-uVolumeHalfSize - uCameraPos) / rayDir;
    vec3 tMax = (uVolumeHalfSize - uCameraPos) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);

    vec3 bgColor = vec3(0.1, 0.1, 0.1);

    if (tFar < 0.0 || tNear > tFar) {
        fragColor = vec4(bgColor, 1.0);
        return;
    }

    float offset = 0.005;
    vec3 accumColor = vec3(0.0);
    float accumAlpha = 0.0;
    float maxDensity = 0.0;
    float minDensity = 1.0;
    bool hit = false;
    vec3 hitNormal = vec3(0.0);
    vec3 hitPos = vec3(0.0);

    vec3 currentPos = vec3(0.0);
    vec3 texCoord = vec3(0.0);
    float density = 0.0;
    vec4 tfSample = vec4(0.0);
    vec3 sampleColor = vec3(0.0);
    float sampleAlpha = 0.0;

    float t = tNear;

    bool wasClipped = false;

    for (int i = 0; i < uMaxSteps; i++) {
        if (t >= tFar) break;
        currentPos = uCameraPos+t*rayDir;

        bool isClipped = uClipEnabled && (dot(currentPos,uClipNormal)+uClipOffset < 0.0);

        if(isClipped){
            wasClipped = true;
            t+= uStepSize;
            continue;
        }

        if (wasClipped && uClipEnabled) {
            // sample density at this boundary point
            texCoord = (currentPos + uVolumeHalfSize) / (uVolumeHalfSize * 2.0);
            density = texture(uVolume, texCoord).r;
            tfSample = texture(uTransferFunction, density);

            if(tfSample.a > 0.05){
                vec3 lightDir = normalize(uLightDir);
                float NdotL = max(dot(-uClipNormal, lightDir), 0.0);
                vec3 cutColor = tfSample.rgb * (0.2 + 0.8 * NdotL);
                accumColor += (1.0 - accumAlpha) * cutColor * tfSample.a;
                accumAlpha += (1.0 - accumAlpha) * tfSample.a;
                
            }
            wasClipped = false;
        }


        currentPos = uCameraPos + t * rayDir;
        texCoord = (currentPos + uVolumeHalfSize) / (uVolumeHalfSize * 2.0);
        density = texture(uVolume, texCoord).r;
        tfSample = texture(uTransferFunction, density);
        sampleColor = tfSample.rgb;
        sampleAlpha = tfSample.a;

        if (uRenderMode == 0) {
            vec3 litColor = sampleColor;
            if (sampleAlpha > 0.001) {
                float posX = texture(uVolume, texCoord + vec3(offset, 0.0, 0.0)).r;
                float negX = texture(uVolume, texCoord - vec3(offset, 0.0, 0.0)).r;
                float posY = texture(uVolume, texCoord + vec3(0.0, offset, 0.0)).r;
                float negY = texture(uVolume, texCoord - vec3(0.0, offset, 0.0)).r;
                float posZ = texture(uVolume, texCoord + vec3(0.0, 0.0, offset)).r;
                float negZ = texture(uVolume, texCoord - vec3(0.0, 0.0, offset)).r;
                vec3 gradient = vec3(posX - negX, posY - negY, posZ - negZ);
                float gradLen = length(gradient);
                if (gradLen > 0.0001) {
                    vec3 normal = gradient / gradLen;
                    vec3 lightDir = normalize(uLightDir);
                    vec3 ambient = 0.2 * sampleColor;
                    float NdotL = max(dot(normal, lightDir), 0.0);
                    vec3 diffuse = NdotL * sampleColor * uLightColor;
                    vec3 viewDir = normalize(uCameraPos - currentPos);
                    vec3 reflectDir = reflect(-lightDir, normal);
                    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
                    vec3 specular = spec * uLightColor * 0.3;
                    litColor = ambient + diffuse + specular;
                }
            }
            accumColor += (1.0 - accumAlpha) * litColor * sampleAlpha;
            accumAlpha += (1.0 - accumAlpha) * sampleAlpha;
            if (accumAlpha > 0.99) break;
            float stepMultiplier = (sampleAlpha < 0.01) ? 4.0 : 1.0;
            t += uStepSize * stepMultiplier;

        } else if (uRenderMode == 1) {
            if (density > 0.1) maxDensity = max(maxDensity, density); 
            
            t += uStepSize;

        } else if (uRenderMode == 2) {
            if (density > 0.1) minDensity = min(minDensity, density);
            
            t += uStepSize;

        } else if (uRenderMode == 3) {
            if (density >= uIsoValue && !hit) {
                float posX = texture(uVolume, texCoord + vec3(offset, 0.0, 0.0)).r;
                float negX = texture(uVolume, texCoord - vec3(offset, 0.0, 0.0)).r;
                float posY = texture(uVolume, texCoord + vec3(0.0, offset, 0.0)).r;
                float negY = texture(uVolume, texCoord - vec3(0.0, offset, 0.0)).r;
                float posZ = texture(uVolume, texCoord + vec3(0.0, 0.0, offset)).r;
                float negZ = texture(uVolume, texCoord - vec3(0.0, 0.0, offset)).r;
                vec3 gradient = vec3(posX - negX, posY - negY, posZ - negZ);
                float gradLen = length(gradient);
                if (gradLen > 0.01) {
                    hit = true;
                    hitNormal = gradient / gradLen;
                    hitPos = currentPos;
                    break;
                }
            }
            t += uStepSize;
        }
    }

    vec3 finalColor = vec3(0.0);
    vec4 isoSample = vec4(0.0);

    switch (uRenderMode) {
        case 0:
            finalColor = accumColor + (1.0 - accumAlpha) * bgColor;
            fragColor = vec4(finalColor, 1.0);
            break;
        case 1:
            if (maxDensity > 0.1)
                fragColor = vec4(vec3(maxDensity), 1.0);
            else
                fragColor = vec4(bgColor, 1.0);
            break;
        case 2:
            if (minDensity < 0.9)
                fragColor = vec4(vec3(minDensity), 1.0);
            else
                fragColor = vec4(bgColor, 1.0);
            break;
        case 3:
            if (hit) {
                isoSample = texture(uTransferFunction, uIsoValue);
                vec3 surfaceColor = isoSample.rgb;
                vec3 lightDir = normalize(uLightDir);
                vec3 ambient = 0.2 * surfaceColor;
                float NdotL = max(dot(hitNormal, lightDir), 0.0);
                vec3 diffuse = NdotL * surfaceColor * uLightColor;
                vec3 viewDir = normalize(uCameraPos - hitPos);
                vec3 reflectDir = reflect(-lightDir, hitNormal);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
                vec3 specular = spec * uLightColor * 0.3;
                fragColor = vec4(ambient + diffuse + specular, 1.0);
            } else {
                fragColor = vec4(bgColor, 1.0);
            }
            break;
        default:
            fragColor = vec4(bgColor, 1.0);
            break;
    }
}