COMPILER=${VULKAN_DIR}/bin/glslangValidator
TARGET_ENV=vulkan1.3

SHADERDIR=shaders
OUTPUTDIR=shaders/spv
mkdir -p ${OUTPUTDIR}

extensions=("vert" "frag")

for extension in ${extensions[@]}
  do
    for FILE in ${SHADERDIR}/*.${extension}
      do
        FILENAME=$(basename ${FILE})
        ${COMPILER} -S ${extension} ${FILE} --target-env ${TARGET_ENV} -o ${OUTPUTDIR}/${FILENAME}.spv
    done
done