import yaml
f = open('./test.yaml')
#print (f.read()) 

doc = """
%TAG ! !hlk
---
  a: "hnk\a"
  b:
     ! c: !!int 245
"""



data = yaml.load(f.read())
f.close()
print (yaml.dump(data, default_flow_style=False))
