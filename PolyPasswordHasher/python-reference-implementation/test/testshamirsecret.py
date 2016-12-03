# import shamirsecret
# from unittest import TestCase

#  Basic tests for this module.   Should output nothing on success.
# class TestShamirSecret(TestCase):

#   def test_math(self):
#     # first check some of the math...
#     self.assertTrue(shamirsecret._multiply_polynomials([1,3,4],[4,5]) == [4,9,31,20])
#     self.assertTrue(shamirsecret._full_lagrange([2,4,5],[14,30,32]) == [43, 168, 150])

#   def test_shamirsecret(self):

#     s = shamirsecret.ShamirSecret(2,'hello')
#     a=s.compute_share(1)
#     b=s.compute_share(2)
#     c=s.compute_share(3)


#     # should be able to recover from any two...
#     t = shamirsecret.ShamirSecret(2)
#     t.recover_secretdata([a,b])

#     t = shamirsecret.ShamirSecret(2)
#     t.recover_secretdata([a,c])

#     t = shamirsecret.ShamirSecret(2)
#     t.recover_secretdata([b,c])

#     # ... or even all three!
#     t = shamirsecret.ShamirSecret(2)
#     t.recover_secretdata([a,b,c])

#     # Try a few examples generated by tss.py

#     #'\x02\x06'
#     #'\x04\xb4'

#     shares = []
#     shares.append((2,bytearray("06".decode("hex"))))
#     shares.append((4,bytearray("b4".decode("hex"))))

#     u = shamirsecret.ShamirSecret(2)
#     u.recover_secretdata(shares)

#     self.assertTrue(u.secretdata == 'h')

#     #'\x03\x1f'
#     #'\x04\xdc'
#     #'\x05\xf1'
#     #'\x06\x86'
#     #'\x07\xab'
#     #'\x08\x1b'
#     shares = []
#     shares.append((3,bytearray("1f".decode("hex"))))
#     shares.append((4,bytearray("dc".decode("hex"))))
#     shares.append((5,bytearray("f1".decode("hex"))))
#     shares.append((6,bytearray("86".decode("hex"))))
#     shares.append((7,bytearray("ab".decode("hex"))))
#     shares.append((8,bytearray("1b".decode("hex"))))


#     u = shamirsecret.ShamirSecret(2)
#     u.recover_secretdata(shares)

#     self.assertTrue(u.secretdata == 'h')


#     #  Try the test from the intro code comment

#     # create a new object with some secret...
#     mysecret = shamirsecret.ShamirSecret(2, 'my shared secret')
#     # get shares out of it...

#     a = mysecret.compute_share(4)
#     b = mysecret.compute_share(6)
#     c = mysecret.compute_share(1)
#     d = mysecret.compute_share(2)

#     # Recover the secret value
#     newsecret = shamirsecret.ShamirSecret(2) 

#     newsecret.recover_secretdata([a,b,c])  # note, two would do...

#     # d should be okay...
#     self.assertTrue(newsecret.is_valid_share(d))

#     # change a byte
#     d[1][3] = (d[1][3] + 1 % 256)

#     # but not now...
#     self.assertTrue(newsecret.is_valid_share(d) is False)

