# DAQ Data Filtering

The PPT device exposes most PPT parameters, but only a few are needed to correctly correct the data (known as gain configuration).  
These are tagged as `record`.

The following macro creates a list based on the tags and update the DAQ filtering.  
Upon setting the filter expression, the DataAggregator recording the PPT needs to have its `policy.enableDataFiltering` set to `True`.  


```python
from typing import Optional, Set
from karabo.middlelayer import *


class SetDAQfilter(Macro):

    @String(
        displayedName="Filter Expression",
        defaultValue="lambda attrs: 'record' in attrs.get('tags', [])"
    )
    def filterExpression(self, value):
        self.filter_expression = eval(value)
        self.filterExpression = value
    
    @Slot()
    async def execute(self):
        async with getDevice("SCS_CDIDET_DSSC/FPGA/PPT_Q1") as proxy:
            print("Getting properties")
            properties = await self.get_properties(schema=None, proxy=proxy)
        print(properties)
        
        async with getDevice("SCS_DAQ_DATA/DM/RUN_CONTROL") as proxy:
            proxy.dataConfiguration.append(Hash(
                "classId", "DsscPpt",
                "behaviour", "include",
                "properties", list(properties))
            )
            

    async def get_properties(self, schema: Optional[Schema] = None, proxy=None) -> Set[str]:
        """Get all properties from the remote device that matches the filter
        expression.
        This will recursively walk through nodes.

        A set of [property names] will be returned
        """
        def recurse(hash_, parent='') -> Set[str]:
            properties = set()

            for key in hash_.keys():
                attrs = hash_.getAttributes(key)

                if attrs['nodeType'] == NodeType.Node:
                    node_props = recurse(hash_[key], parent=key)
                    if node_props:
                        properties.update(node_props)

                elif self.filter_expression(attrs):
                    if parent:
                        key = ".".join((parent, key))
                    properties.add(key)

            return properties

        if schema is None:
            schema = (await getSchema(proxy))

        properties = recurse(schema.hash)

        return properties
```
