rm *.hlsl
rm default/*.hlsl

path_glslcc=../../../../glsl_to_hlsl/glslcc.exe

declare -a types=('basic' 'debug' 'deferred' 'sky' 'sprite' 'unlit' 'default/empty')

profile=420
for i in "${types[@]}"; do
	echo "#version ${profile}" > "${i}_vs.glsl_tmp"
	echo "#version ${profile}" > "${i}_fs.glsl_tmp"
	cat "${i}_vs.glsl" >> "${i}_vs.glsl_tmp"
	cat "${i}_fs.glsl" >> "${i}_fs.glsl_tmp"
	$path_glslcc --lang=hlsl --output "${i}.hlsl" \
		--vert "${i}_vs.glsl_tmp" \
		--frag "${i}_fs.glsl_tmp"
	echo ""
	rm "${i}_vs.glsl_tmp"
	rm "${i}_fs.glsl_tmp"
done
