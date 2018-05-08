   {
    "targets": [
      {
        "target_name": "wickrio_addon",
        "sources": [ "wickrio_addon.cc" ],
        "include_dirs" : [
          "<!(node -e \"require('nan')\")"
        ]
      }
    ]
  }
