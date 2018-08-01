import numpy as np

'''
Script to make params.dat for hw-sw gps example
----------------------------------------------

params[(2*Nsta*Nsta)+(Mobs*Mobs)]:
		1 - P[Nsta*Nsta]
		2 - Q[Nsta*Nsta]
		3 - R[Mobs*Mobs]
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

    R = np.eye(M)*rval
    Q = np.eye(N)*qval
    F = initF()
    P = initP(pval)

    params = np.concatenate((P.flatten(), Q.flatten(), R.flatten()), axis=0)

    dump = ""

    for i, x in enumerate(params):
        val = toFixed(x)
        pstring = "params[%d] = (uint32_t) %d;\n"%(i, val)
        dump += pstring

    for i, x in enumerate(F.flatten()):
        val = toFixed(x)
        pstring = "F_i[%d] = (uint32_t) %d;\n"%(i, val)
        dump += pstring

    with open("params.dat", "w") as f:
        f.write(dump)

