import sympy
from sympy.algebras.quaternion import Quaternion

cx,sx,cy,sy,cz,sz = sympy.symbols("cx sx cy sy cz sz")

qx = Quaternion(cx, sx, 0, 0)
qy = Quaternion(cy, 0, sy, 0)
qz = Quaternion(cz, 0, 0, sz)
qs = { "X": qx, "Y": qy, "Z": qz }

orders = "XYZ XZY YZX YXZ ZXY ZYX".split()

print("\tswitch (order) {")
for order in orders:
    q = qs[order[2]] * qs[order[1]] * qs[order[0]]
    print("\tcase UFBX_ROTATION_{}:".format(order))
    print("\t\tq.x = {};".format(q.b))
    print("\t\tq.y = {};".format(q.c))
    print("\t\tq.z = {};".format(q.d))
    print("\t\tq.w = {};".format(q.a))
    print("\t\tbreak;")
print("\tdefault:")
print("\t\tq.x = q.y = q.z = 0.0; q.w = 1.0;")
print("\t\tbreak;")
print("\t}")    
