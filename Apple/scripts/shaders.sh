#!/bin/bash

# Builds the game's GLSL into the MSL of each stage; the device builds their libraries

set -e
cd "$(dirname "$0")"
SHADERS=../../client/shaders

# The deployment target of the check build
if [ "$1" = ios ]; then
	. ../../iOS/deps/sdk.sh
	SDK=$XCODE_SDK
	DEPLOY=$MIN_VERSION
	DEST=../../iOS/assets/client/shaders/Metal
else
	. ./sdk.sh
	SDK=macosx
	DEPLOY=-mmacosx-version-min=$OSX_OSVER
	DEST=../assets/client/shaders/Metal
fi

for tool in glslangValidator spirv-cross; do
	command -v $tool >/dev/null || { echo "$tool not found: brew install glslang spirv-cross" >&2; exit 1; }
done

OUT=$(mktemp -d)

# SETTINGS_UNIFORMS picks the uniform path, the material types mirror MaterialType in src/client/tile.h
cat > "$OUT/defs.glsl" <<'EOF'
#define SETTINGS_UNIFORMS 1
#define TILE_MATERIAL_WAVING_LEAVES 4
#define TILE_MATERIAL_WAVING_PLANTS 5
#define TILE_MATERIAL_WAVING_LIQUID_BASIC 7
#define TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT 8
#define TILE_MATERIAL_WAVING_LIQUID_OPAQUE 9
#define TILE_MATERIAL_PLAIN 10
#define TILE_MATERIAL_PLAIN_ALPHA 11
EOF

# Slots as the driver declares the vertex layout
cat > "$OUT/head_vs.glsl" <<'EOF'
#version 310 es
precision highp float;

uniform highp mat4 mWorldView;
uniform highp mat4 mWorldViewProj;
uniform highp mat4 mTexture;

layout(location = 0) in highp vec4 inVertexPosition;
layout(location = 1) in mediump vec3 inVertexNormal;
layout(location = 2) in lowp vec4 inVertexColor;
layout(location = 3) in highp vec4 inTexCoord0;
layout(location = 4) in mediump vec4 inTexCoord1;
layout(location = 5) in mediump vec4 inVertexTangent;
layout(location = 6) in mediump vec4 inVertexBinormal;
EOF

cat > "$OUT/head_fs.glsl" <<'EOF'
#version 310 es
precision mediump float;
layout(location = 0) out vec4 fragColor;
EOF

# ES 3 dropped the words the game's shaders are written in, and Metal has no gl_FragColor
prepare() {
	# A varying OpenGL reads at the centroid keeps centroid
	centroid='s/^varying \(.*varTexCoord;\)$/'"$2"' \1/'
	grep -q '^centroid varying vec2 varTexCoord;$' "$1" && centroid='s/^varying \(.*varTexCoord;\)$/centroid '"$2"' \1/'
	sed -e "$centroid" \
		-e 's/^varying /'"$2"' /' \
		-e 's/texture2D(/texture(/g' \
		-e 's/textureCube(/texture(/g' \
		-e 's/gl_FragColor/fragColor/g' "$1"
}

build_stage() {
	tag=$1; dir=$2; stage=$3; extra=$4
	if [ "$stage" = vert ]; then
		src=opengl_vertex.glsl; head="$OUT/head_vs.glsl"; word=out; prefix=vs
	else
		src=opengl_fragment.glsl; head="$OUT/head_fs.glsl"; word=in; prefix=fs
	fi

	{ cat "$head"; echo "$extra"; cat "$OUT/defs.glsl"; \
		prepare "$SHADERS/$dir/$src" "$word"; } > "$OUT/${prefix}_${tag}.glsl"

	glslangValidator -G -S "$stage" --auto-map-locations \
		-o "$OUT/${prefix}_${tag}.spv" "$OUT/${prefix}_${tag}.glsl" > "$OUT/${prefix}_${tag}.log" 2>&1 || {
			echo "GLSL did not compile for ${prefix}_${tag}:" >&2
			cat "$OUT/${prefix}_${tag}.log" >&2; exit 1; }

	spirv-cross --msl --msl-version 20100 \
		--rename-entry-point main "${prefix}_${tag}" "$stage" \
		"$OUT/${prefix}_${tag}.spv" --output "$OUT/${prefix}_${tag}.metal"

	# A check only: the device builds the library
	xcrun -sdk "$SDK" metal $DEPLOY -c "$OUT/${prefix}_${tag}.metal" \
		-o "$OUT/${prefix}_${tag}.air"
}

# The programs, named the way shaderVariant() names them
: > "$OUT/vertex.txt"
: > "$OUT/fragment.txt"
: > "$OUT/programs.txt"

LIGHTS=$(sed -n 's/^constexpr u32 MAX_POINT_LIGHTS = \([0-9][0-9]*\);.*/\1/p' ../../src/client/shader.h)
[ -n "$LIGHTS" ] || { echo "MAX_POINT_LIGHTS not found in src/client/shader.h" >&2; exit 1; }

# A waving branch and the material types it covers, as a discard mode each
for branch in ":|0|0|0|0|solid blend ref" \
              ":leaves|4|0|1|0|ref" \
              ":plants|5|0|0|1|ref" \
              ":liquid|8|1|0|0|solid blend ref"; do
	IFS='|' read -r name mt water leaves plants modes <<< "$branch"
	name=${name#:}
	for lights in 0 $LIGHTS; do
		if [ "$lights" = 0 ]; then lname=""; lsuffix=""; else lname=":lights"; lsuffix="_lights"; fi
		vtag="nodes${name:+_$name}$lsuffix"
		echo "$vtag|nodes_shader|$mt|$water|$leaves|$plants|$lights" >> "$OUT/vertex.txt"
		for mode in $modes; do
			[ "$mode" = solid ] && dname="" || dname=":$mode"
			for tone in 0 1; do
				if [ "$tone" = 0 ]; then tname=""; tsuffix=""; else tname=":tonemap"; tsuffix="_tonemap"; fi
				ftag="nodes_${mode}${tsuffix}${lsuffix}"
				echo "$ftag|nodes_shader|$mode|$tone|$lights" >> "$OUT/fragment.txt"
				echo "nodes_shader${name:+:$name}$dname$tname$lname|$vtag|$ftag" \
						>> "$OUT/programs.txt"
			done
		done
	done
done

for branch in ":|0|solid blend ref" ":plain|10|blend ref"; do
	IFS='|' read -r name mt modes <<< "$branch"
	name=${name#:}
	vtag="object${name:+_$name}"
	echo "$vtag|object_shader|$mt|0|0|0|0" >> "$OUT/vertex.txt"
	for mode in $modes; do
		[ "$mode" = solid ] && dname="" || dname=":$mode"
		for tone in 0 1; do
			if [ "$tone" = 0 ]; then tname=""; tsuffix=""; else tname=":tonemap"; tsuffix="_tonemap"; fi
			ftag="object_${mode}${tsuffix}"
			echo "$ftag|object_shader|$mode|$tone|0" >> "$OUT/fragment.txt"
			echo "object_shader${name:+:$name}$dname$tname|$vtag|$ftag" \
					>> "$OUT/programs.txt"
		done
	done
done

for one in selection stars minimap; do
	echo "$one|${one}_shader|0|0|0|0|0" >> "$OUT/vertex.txt"
	echo "$one|${one}_shader|solid|0|0" >> "$OUT/fragment.txt"
	echo "${one}_shader|$one|$one" >> "$OUT/programs.txt"
done

sort -u "$OUT/vertex.txt" -o "$OUT/vertex.txt"
sort -u "$OUT/fragment.txt" -o "$OUT/fragment.txt"

while IFS='|' read -r tag dir mt water leaves plants lights; do
	[ -n "$tag" ] || continue
	build_stage "$tag" "$dir" vert "#define MATERIAL_TYPE $mt
#define ENABLE_WAVING_WATER $water
#define ENABLE_WAVING_LEAVES $leaves
#define ENABLE_WAVING_PLANTS $plants
#define POINT_LIGHTS $lights
#define ENABLE_TONE_MAPPING 0"
	echo "built vs_$tag"
done < "$OUT/vertex.txt"

while IFS='|' read -r tag dir mode tone lights; do
	[ -n "$tag" ] || continue
	case "$mode" in
		blend) discard="#define USE_ALPHA_REF_DISCARD 1" ;;
		ref) discard="#define USE_ALPHA_TEST_DISCARD 1" ;;
		*) discard="" ;;
	esac
	build_stage "$tag" "$dir" frag "$discard
#define MATERIAL_TYPE 0
#define ENABLE_WAVING_WATER 0
#define ENABLE_WAVING_LEAVES 0
#define ENABLE_WAVING_PLANTS 0
#define POINT_LIGHTS $lights
#define ENABLE_TONE_MAPPING $tone"
	echo "built fs_$tag"
done < "$OUT/fragment.txt"

# Drops the stages and library of an older build
mkdir -p "$DEST"
rm -f "$DEST"/*.metal "$DEST/metal_shaders.metallib"
cp "$OUT"/vs_*.metal "$OUT"/fs_*.metal "$OUT/programs.txt" "$DEST/"

echo "stages: $(($(ls "$DEST"/*.metal | wc -l))), programs: $(($(wc -l < "$DEST/programs.txt")))"
rm -rf "$OUT"
