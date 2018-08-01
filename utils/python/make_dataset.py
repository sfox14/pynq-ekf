import numpy as np
from math import sin, cos, pi, radians
import csv

np.random.seed(43)


# linear motion
def linear_motion(x0, grad, N=50):
    data = []
    for i in range(N):
        x0 = x0 + np.array(grad)
        data.append(x0)
    return np.array(data)


# circular motion
def circular_motion(center, radius=10, omega=0.1, N=50):
    """
    centre: (x, y, z) centre of rotation
    radius: radius of rotation
    omega: angular velocity
    """
    sgn = np.random.randint(2, size=3) * 2 - 1
    center_of_rotation_x = center[0]
    center_of_rotation_y = center[1]
    center_of_rotation_z = center[2]
    angle = radians(45)  # pi/4 # starting angle 45 degrees

    x = center_of_rotation_x + sgn[0] * radius * cos(
        angle)  # Starting position x
    y = center_of_rotation_y + sgn[1] * radius * sin(
        angle)  # Starting position y
    z = center_of_rotation_z + sgn[2] * radius * cos(
        angle)  # Starting position y

    data = np.zeros((N, 3))
    for i in range(N):
        angle = angle + omega  # New angle, we add angular velocity
        x = x + sgn[0] * radius * omega * cos(angle + pi / 2)  # New x
        y = y + sgn[1] * radius * omega * sin(angle + pi / 2)  # New y
        z = z + sgn[2] * radius * omega * cos(angle + pi / 4)  # New z
        data[i][0] = x
        data[i][1] = y
        data[i][2] = z
    return data


def euclidean_distance(x0, x1, bias=0):
    dx = x0 - x1
    dx2 = dx ** 2
    dist = np.sqrt(np.sum(dx2, axis=1)) + bias
    return dist


def make():
    # create receiver positions
    rx0 = np.random.randn(3)  # initial position
    rx_data = linear_motion(rx0, [0.2, -0.2, 0.2], N=50)

    # create satellite positions
    st0 = np.random.randn(4, 3)
    # sat1 = linear_motion(st0[0, :], [0.08, -0.1, 0.02], N=100)
    # sat2 = linear_motion(st0[1, :], [0.02, 0.1, -0.2], N=100)
    # sat3 = linear_motion(st0[2, :], [-0.4, 0.1, 0.02], N=100)
    # sat4 = linear_motion(st0[3, :], [0.3, -0.1, 0.22], N=100)

    sat1 = circular_motion(st0[0, :], N=50)
    sat2 = circular_motion(st0[1, :], N=50)
    sat3 = circular_motion(st0[2, :], N=50)
    sat4 = circular_motion(st0[3, :], N=50)

    # calculate the observed rho
    rho1 = euclidean_distance(rx_data, sat1, bias=0.02)
    rho2 = euclidean_distance(rx_data, sat2, bias=0.02)
    rho3 = euclidean_distance(rx_data, sat3, bias=0.02)
    rho4 = euclidean_distance(rx_data, sat4, bias=0.02)

    # add white noise to measurements
    sigma = 5e-1
    mu = 0.0
    sat1 = sat1 + np.random.normal(mu, sigma, (50, 3))
    sat2 = sat2 + np.random.normal(mu, sigma, (50, 3))
    sat3 = sat3 + np.random.normal(mu, sigma, (50, 3))
    sat4 = sat4 + np.random.normal(mu, sigma, (50, 3))
    rho1 = rho1 + np.random.normal(mu, sigma, 50)
    rho2 = rho2 + np.random.normal(mu, sigma, 50)
    rho3 = rho3 + np.random.normal(mu, sigma, 50)
    rho4 = rho4 + np.random.normal(mu, sigma, 50)

    # merge dataset
    data = np.hstack((sat1, sat2))
    data = np.hstack((data, sat3))
    data = np.hstack((data, sat4))
    rho = np.array([rho1, rho2, rho3, rho4]).T
    data = np.hstack((data, rho)).astype(np.float32)

    header = "x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, r1, r2, r3, r4"
    np.savetxt("gps_data.csv", data, fmt="%.6f", delimiter=",", header=header,
               comments="")


if __name__ == "__main__":
    make()
