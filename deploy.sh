
# Load variables from .env
set -a
. ./.env
set +a

# Check that build folder exists
if [ ! -d "$BUILD_DIR" ]; then
    printf "Build directory missing: ${BUILD_DIR}\n"
    exit 1
fi

# Strip operating system line endings
RPI_USER=${RPI_USER//$'\r'/}
RPI_IP_ADDR=${RPI_IP_ADDR//$'\r'/}

# Transfer files to raspberry pi
printf "Target: ${RPI_USER}@${RPI_IP_ADDR}\n"

scp -O \
   -o MACs=hmac-sha2-256 \
    "$BUILD_DIR/alarm-manager" \
    "$BUILD_DIR/pulse-oximeter" \
    "$RPI_USER@$RPI_IP_ADDR:/home/qnxuser/"