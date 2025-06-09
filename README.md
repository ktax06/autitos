docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t arturoalvarez/api-autitos:latest \
  --push .