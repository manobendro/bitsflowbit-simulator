import {
  BITSFLOW_HAL_ACCELEROMETER_EVT_2G,
  BITSFLOW_HAL_ACCELEROMETER_EVT_3G,
  BITSFLOW_HAL_ACCELEROMETER_EVT_6G,
  BITSFLOW_HAL_ACCELEROMETER_EVT_8G,
  BITSFLOW_HAL_ACCELEROMETER_EVT_FACE_DOWN,
  BITSFLOW_HAL_ACCELEROMETER_EVT_FACE_UP,
  BITSFLOW_HAL_ACCELEROMETER_EVT_FREEFALL,
  BITSFLOW_HAL_ACCELEROMETER_EVT_NONE,
  BITSFLOW_HAL_ACCELEROMETER_EVT_SHAKE,
  BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_DOWN,
  BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_LEFT,
  BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_RIGHT,
  BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_UP,
  BITSFLOW_HAL_MICROPHONE_SET_THRESHOLD_HIGH,
  BITSFLOW_HAL_MICROPHONE_SET_THRESHOLD_LOW,
} from "./constants";

export function convertSoundThresholdNumberToString(
  value: number
): "low" | "high" {
  switch (value) {
    case BITSFLOW_HAL_MICROPHONE_SET_THRESHOLD_LOW:
      return "low";
    case BITSFLOW_HAL_MICROPHONE_SET_THRESHOLD_HIGH:
      return "high";
    default:
      throw new Error(`Invalid value ${value}`);
  }
}

export function convertAccelerometerStringToNumber(value: string): number {
  switch (value) {
    case "none":
      return BITSFLOW_HAL_ACCELEROMETER_EVT_NONE;
    case "up":
      return BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_UP;
    case "down":
      return BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_DOWN;
    case "left":
      return BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_LEFT;
    case "right":
      return BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_RIGHT;
    case "face up":
      return BITSFLOW_HAL_ACCELEROMETER_EVT_FACE_UP;
    case "face down":
      return BITSFLOW_HAL_ACCELEROMETER_EVT_FACE_DOWN;
    case "freefall":
      return BITSFLOW_HAL_ACCELEROMETER_EVT_FREEFALL;
    case "2g":
      return BITSFLOW_HAL_ACCELEROMETER_EVT_2G;
    case "3g":
      return BITSFLOW_HAL_ACCELEROMETER_EVT_3G;
    case "6g":
      return BITSFLOW_HAL_ACCELEROMETER_EVT_6G;
    case "8g":
      return BITSFLOW_HAL_ACCELEROMETER_EVT_8G;
    case "shake":
      return BITSFLOW_HAL_ACCELEROMETER_EVT_SHAKE;
    default:
      throw new Error(`Invalid value ${value}`);
  }
}

export function convertAccelerometerNumberToString(value: number): string {
  switch (value) {
    case BITSFLOW_HAL_ACCELEROMETER_EVT_NONE:
      return "none";
    case BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_UP:
      return "up";
    case BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_DOWN:
      return "down";
    case BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_LEFT:
      return "left";
    case BITSFLOW_HAL_ACCELEROMETER_EVT_TILT_RIGHT:
      return "right";
    case BITSFLOW_HAL_ACCELEROMETER_EVT_FACE_UP:
      return "face up";
    case BITSFLOW_HAL_ACCELEROMETER_EVT_FACE_DOWN:
      return "face down";
    case BITSFLOW_HAL_ACCELEROMETER_EVT_FREEFALL:
      return "freefall";
    case BITSFLOW_HAL_ACCELEROMETER_EVT_2G:
      return "2g";
    case BITSFLOW_HAL_ACCELEROMETER_EVT_3G:
      return "3g";
    case BITSFLOW_HAL_ACCELEROMETER_EVT_6G:
      return "6g";
    case BITSFLOW_HAL_ACCELEROMETER_EVT_8G:
      return "8g";
    case BITSFLOW_HAL_ACCELEROMETER_EVT_SHAKE:
      return "shake";
    default:
      throw new Error(`Invalid value ${value}`);
  }
}

export const convertAudioBuffer = (
  heap: Uint8Array,
  source: number,
  target: AudioBuffer
) => {
  const channel = target.getChannelData(0);
  for (let i = 0; i < channel.length; ++i) {
    // Convert from uint8 to -1..+1 float.
    channel[i] = (heap[source + i] / 255) * 2 - 1;
  }
  return target;
};
