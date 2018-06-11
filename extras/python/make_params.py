import numpy as np

'''
params[Nsta+Nsta+Mobs+(2*Nsta*Nsta)+(Mobs*Nsta)+2]:
		1 - x[Nsta]
		2 - fx[Nsta]
		3 - hx[Mobs]
		4 - F[Nsta*Nsta]
		5 - H[Mobs*Nsta]
		6 - P[Nsta*Nsta]
		7 - qval
		8 - rval
'''


def toFixed(a):
    val = (1 << 20)
    if a < 0:
        result = (a * val) - 0.5
    else:
        result = (a * val) + 0.5
    return int(result)

def initF():
    a = np.array([[1,1],[0,1]])
    return np.kron(np.eye(4), a)

def initP(pval=0.5):
    return np.eye(N)*pval

if __name__ == "__main__":

    N = 8
    M = 4

    pval = 0.5
    qval = 0.1
    rval = 20.0

    x = np.array([0.25739993, 0.3, -0.90848143, -0.1, -0.37850311, 0.3, 0.02, 0])

    F = initF()
    H = np.zeros(shape=(M,N))
    P = initP(pval)

    fx = np.zeros(N)
    hx = np.zeros(M)

    params = np.concatenate((x, fx, hx, F.flatten(), H.flatten(), P.flatten(),
                             np.array([qval]), np.array([rval])), axis=0)

    dump = ""

    for i, x in enumerate(params):
        val = toFixed(x)
        pstring = "params[%d] = (uint32_t) %d;\n"%(i, val)
        dump += pstring

    with open("params.dat", "w") as f:
        f.write(dump)

