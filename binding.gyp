{
  "targets": [
    {
      "target_name": "netroute",

      "include_dirs": [
        "src",
      ],

      "sources": [
        "src/netroute.cc",
      ],

      "conditions": [
        ["OS == 'linux'", {
          "sources": ["src/platform-linux.cc"],
        }, {
          "sources": ["src/platform-unix.cc"],
        }],
      ],
    }
  ]
}
