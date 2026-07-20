from sympy import *
from sympy.matrices import *
from sympy.algebras.quaternion import Quaternion

qx, qy, qz = symbols("q.x q.y q.z", real=True)
vx, vy, vz = symbols("v.x v.y v.z", real=True)
qw = symbols("q.w", real=True)

qv = Matrix([qx, qy, qz])
v = Matrix([vx, vy, vz])

# https://fgiesen.wordpress.com/2019/02/09/rotating-a-single-vector-using-a-quaternion/
t = (2.0*qv).cross(v)
r = v + qw*t + qv.cross(t)

r = simplify(r)

for a in range(3):
    for b in range(a+1, 3):
        an = "xyz"[a]
        bn = "xyz"[b]
        e = qv[a]*v[b] - qv[b]*v[a]
        s = symbols(an + bn)
        print("ufbx_real {} = {};".format(s, e))
        r = r.subs(e, s)

print("ufbx_vec3 r;")

def sgns(s):
    return "+" if s >= 0 else "-"

for a in range(3):
    an = "xyz"[a]
    ex, ey, ez = qx, qy, qz
    if a == 0: ex = qw
    if a == 1: ey = qw
    if a == 2: ez = qw
    sx, x = simplify(ex*r[a].coeff(ex) / 2).as_coeff_Mul()
    sy, y = simplify(ey*r[a].coeff(ey) / 2).as_coeff_Mul()
    sz, z = simplify(ez*r[a].coeff(ez) / 2).as_coeff_Mul()
    assert abs(sx) == 1 and abs(sy) == 1 and abs(sz) == 1
    w = simplify(r[a] - 2*(sx*x+sy*y+sz*z))
    print("r.{} = 2.0 * ({} {} {} {} {} {}) + {};".format(an, sgns(sx), x, sgns(sy), y, sgns(sz), z, w))

print()

ax, ay, az, aw = symbols("a.x a.y a.z a.w", real=True)
bx, by, bz, bw = symbols("b.x b.y b.z b.w", real=True)
qa = Quaternion(aw, ax, ay, az)
qb = Quaternion(bw, bx, by, bz)

qr = qa*qb
print("ufbx_vec4 r;")
print("r.x = {};".format(qr.b))
print("r.y = {};".format(qr.c))
print("r.z = {};".format(qr.d))
print("r.w = {};".format(qr.a))

print()

# Unit quaternion
qx, qy, qz, qw = symbols("q.x q.y q.z q.w", real=True)
qq = Quaternion(qw, qx, qy, qz)
ma = qq.to_rotation_matrix()
ma = ma.subs(qw**2 + qx**2 + qy**2 + qz**2, 1)

qc = (qx, qy, qz, qw)

print("ufbx_vec4 q = t->rotation;")
print("ufbx_real ", end="")
for a in range(3):
    if a != 0: print(", ", end="")
    an = "xyz"[a]
    print("s{0} = 2.0 * t->scale.{0}".format(an), end="")
print(";")

for a in range(4):
    print("ufbx_real ", end="")
    for b in range(a, 4):
        an = "xyzw"[a]
        bn = "xyzw"[b]
        e = qc[a]*qc[b]
        s = an + bn
        if b != a: print(", ", end="")
        print("{} = {}*{}".format(s, qc[a], qc[b]), end="")

        ma = ma.subs(e, s)
    print(";")

print("ufbx_matrix m;")
for c in range(3):
    for r in range(3):
        e = ma[r,c]
        t, t12 = e.as_coeff_Add()
        s1, e1 = t12.args[0].as_coeff_Mul()
        s2, e2 = t12.args[1].as_coeff_Mul()
        assert abs(s1) == 2 and abs(s2) == 2
        assert t == 0 or t == 1
        ts = " + 0.5" if t else ""
        sx = "s" + "xyz"[c]
        print("m.m{}{} = {} * ({} {} {} {}{});".format(r, c, sx, sgns(s1), e1, sgns(s2), e2, ts))
for r in range(3):
    rn = "xyz"[r]
    print("m.m{}3 = t->translation.{};".format(r, rn))
print("return m;")
