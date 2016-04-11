'use strict';

module.exports = function(grunt) {

    grunt.initConfig({
        dirs: {
            dest: 'dist',
            src: 'source'
        },

        // Copy files that don't need compilation to dist/
        copy: {
            dist: {
                files: [
                    { dest: '<%= dirs.dest %>/', src: 'assets/js/vendor/jquery*.min.js', expand: true, cwd: '<%= dirs.src %>/' }
                ]
            }
        },

        jekyll: {
            site: {
                options: {
                    bundleExec: true,
                    incremental: false
                }
            }
        },

        htmlmin: {
            dist: {
                options: {
                    collapseBooleanAttributes: true,
                    collapseInlineTagWhitespace: false,
                    collapseWhitespace: true,
                    conservativeCollapse: false,
                    decodeEntities: true,
                    ignoreCustomComments: [/^\s*google(off|on):\s/],
                    minifyCSS: {
                        //advanced: false,
                        compatibility: 'ie8',
                        keepSpecialComments: 0
                    },
                    minifyJS: true,
                    minifyURLs: false,
                    processConditionalComments: true,
                    removeAttributeQuotes: true,
                    removeComments: true,
                    removeOptionalAttributes: true,
                    removeOptionalTags: true,
                    removeRedundantAttributes: true,
                    removeScriptTypeAttributes: true,
                    removeStyleLinkTypeAttributes: true,
                    removeTagWhitespace: false,
                    sortAttributes: true,
                    sortClassName: true
                },
                expand: true,
                cwd: '<%= dirs.dest %>',
                dest: '<%= dirs.dest %>',
                src: ['**/*.html']
            }
        },

        concat: {
            css: {
                src: ['<%= dirs.src %>/assets/css/vendor/normalize.css',
                      '<%= dirs.src %>/assets/css/vendor/jquery.fancybox.css',
                      '<%= dirs.src %>/assets/css/vendor/highlighter.css',
                      '<%= dirs.src %>/assets/css/flags-sprite.css',
                      '<%= dirs.src %>/assets/css/style.css'
                ],
                dest: '<%= dirs.dest %>/assets/css/pack.css'
            },
            mainJs: {
              src: ['<%= dirs.src %>/assets/js/no-js-class.js',
                    '<%= dirs.src %>/assets/js/onLoad.js',
                    '<%= dirs.src %>/assets/js/google-analytics.js'
              ],
              dest: '<%= dirs.dest %>/assets/js/main.js'
            },
            fancybox: {
                src: ['<%= dirs.src %>/assets/js/vendor/plugins.js',
                      '<%= dirs.src %>/assets/js/vendor/jquery.mousewheel.js',
                      '<%= dirs.src %>/assets/js/vendor/jquery.fancybox.js',
                      '<%= dirs.src %>/assets/js/fancybox-init.js'
                ],
                dest: '<%= dirs.dest %>/assets/js/fancybox.js'
            }
        },

        uncss: {
            options: {
                htmlroot: '<%= dirs.dest %>',
                ignore: [
                    /(#|\.)fancybox(\-[a-zA-Z]+)?/,
                    /\.no\-js/
                ],
                ignoreSheets: [/fonts.googleapis/, /www.google.com/, /pagead2.googlesyndication.com/],
                stylesheets: ['/assets/css/pack.css']
            },
            dist: {
                src: '<%= dirs.dest %>/**/*.html',
                dest: '<%= concat.css.dest %>'
            }
        },

        cssmin: {
            minify: {
                options: {
                    compatibility: 'ie8',
                    keepSpecialComments: 0
                },
                files: {
                    '<%= uncss.dist.dest %>': '<%= concat.css.dest %>'
                }
            }
        },

        uglify: {
            options: {
                compress: {
                    warnings: false
                },
                mangle: true,
                preserveComments: false
            },
            minify: {
                files: {
                    '<%= concat.fancybox.dest %>': '<%= concat.fancybox.dest %>',
                    '<%= concat.mainJs.dest %>': '<%= concat.mainJs.dest %>'
                }
            }
        },

        filerev: {
            css: {
                src: '<%= dirs.dest %>/assets/css/**/*.css'
             },
            js: {
                src: [
                    '<%= dirs.dest %>/assets/js/**/*.js',
                    '!<%= dirs.dest %>/assets/js/vendor/jquery*.min.js'
                ]
            },
            images: {
                src: [
                    '<%= dirs.dest %>/assets/img/**/*.{jpg,jpeg,gif,png,svg}',
                    '!<%= dirs.dest %>/assets/img/logo-256x256.png'
                ]
            }
        },

        useminPrepare: {
            html: '<%= dirs.dest %>/index.html',
            options: {
                dest: '<%= dirs.dest %>',
                root: '<%= dirs.dest %>'
            }
        },

        usemin: {
            css: '<%= dirs.dest %>/assets/css/pack*.css',
            html: '<%= dirs.dest %>/**/*.html',
            options: {
                assetsDirs: ['<%= dirs.dest %>/', '<%= dirs.dest %>/assets/img/']
            }
        },

        connect: {
            options: {
                hostname: 'localhost',
                livereload: 35729,
                port: 8000
            },
            livereload: {
                options: {
                    base: '<%= dirs.dest %>/',
                    open: true  // Automatically open the webpage in the default browser
                }
            }
        },

        watch: {
            options: {
                livereload: '<%= connect.options.livereload %>'
            },
            dev: {
                files: ['<%= dirs.src %>/**', '.jshintrc', '_config.yml', 'Gruntfile.js'],
                tasks: 'dev'
            },
            build: {
                files: ['<%= dirs.src %>/**', '.jshintrc', '_config.yml', 'Gruntfile.js'],
                tasks: 'build'
            }
        },

        csslint: {
            options: {
                csslintrc: '.csslintrc'
            },
            src: '<%= dirs.src %>/assets/css/style.css'
        },

        jshint: {
            options: {
                jshintrc: '.jshintrc'
            },
            files: {
                src: ['Gruntfile.js', '<%= dirs.src %>/assets/js/*.js', '!<%= dirs.src %>/assets/js/google-analytics.js']
            }
        },

        htmllint: {
            src: '<%= dirs.dest %>/**/*.html'
        },

        clean: {
            dist: '<%= dirs.dest %>/'
        }

    });

    // Load any grunt plugins found in package.json.
    require('load-grunt-tasks')(grunt, {scope: 'devDependencies'});
    require('time-grunt')(grunt);

    grunt.registerTask('build', [
        'clean',
        'jekyll',
        'useminPrepare',
        'copy',
        'concat',
        'uncss',
        'cssmin',
        'uglify',
        'filerev',
        'usemin',
        'htmlmin'
    ]);

    grunt.registerTask('test', [
        'build',
        'csslint',
        'jshint',
        'htmllint'
    ]);

    grunt.registerTask('dev', [
        'jekyll',
        'useminPrepare',
        'copy',
        'concat',
        'filerev',
        'usemin'
    ]);

    grunt.registerTask('server', [
        'build',
        'connect',
        'watch:build'
    ]);

    grunt.registerTask('default', [
        'dev',
        'connect',
        'watch:dev'
    ]);

};
