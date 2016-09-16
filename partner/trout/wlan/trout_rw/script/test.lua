--set mode----

trw.set_mode(1)
print(string.format("courrent config mode is : %s",trw.get_mode()))

--open power-----


trw.set_power(1,1)


--read and write reg.------

print("test read reg:  ")
addr = 0x8038
for i=0,9,1 do
 val = trw.get_reg(addr)
 print(string.format("%x=%x",addr,val))
 addr = addr + i
end

print("test write reg:")

STA_ADDR = 0x8018
STA_VAL = 0x12345678

trw.set_reg(STA_VAL,STA_ADDR)

val = trw.get_reg(STA_ADDR)
if val == STA_VAL then
print("write reg success")
else
print("write reg failed")
end

--read and write phy reg.------

PHY_ADDR= 0x8c
PHY_VAL = 0x01

trw.set_phy(PHY_VAL,PHY_ADDR)
val = trw.get_phy(PHY_ADDR)
if val== PHY_VAL then
print("write phy reg success")
else
print("write phy reg failed")
end

--close power------
trw.set_power(0,1)

--quit----------
os.exit(0)
