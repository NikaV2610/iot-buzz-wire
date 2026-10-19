import cv2
import numpy as np

camera = cv2.VideoCapture(0)

try:
    if not camera.isOpened():
        raise RuntimeError("Kamera konnte nicht geöffnet werden.")

    while True:
        success, frame = camera.read()

        if not success:
            print("Kein Kamerabild empfangen.")
            break

        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

        mask = cv2.inRange(
            hsv,
            np.array([100, 120, 30]),
            np.array([130, 255, 255])
        )

        # Kleine Störungen entfernen.
        kernel = np.ones((3, 3), dtype=np.uint8)
        mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)

        contours, _ = cv2.findContours(
            mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
        )

        if contours:
            largest = max(contours, key=cv2.contourArea)

            if cv2.contourArea(largest) > 10:
                moments = cv2.moments(largest)

                if moments["m00"] != 0:
                    x = int(moments["m10"] / moments["m00"])
                    y = int(moments["m01"] / moments["m00"])

                    print(f"X: {x}, Y: {y}")

                    cv2.drawContours(frame, [largest], -1, (0, 255, 0), 2)
                    cv2.circle(frame, (x, y), 6, (0, 255, 0), -1)
                    cv2.putText(
                        frame,
                        f"X: {x}  Y: {y}",
                        (20, 35),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        0.8,
                        (0, 255, 0),
                        2,
                    )

        cv2.imshow("Buzz Wire Kamera - Q zum Beenden", frame)
        cv2.imshow("Erkannte rote Bereiche", mask)

        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

finally:
    camera.release()
    cv2.destroyAllWindows()