'use strict';

module.exports = function(grunt) {

    grunt.initConfig({
        dirs: {
            dest: 'dist',
            src: 'source'
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
                        compatibility: 'ie9',
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
                      '<%= dirs.src %>/assets/css/vendor/baguetteBox.css',
                      '<%= dirs.src %>/assets/css/vendor/highlighter.css',
                      '<%= dirs.src %>/assets/css/flags-sprite.css',
                      '<%= dirs.src %>/assets/css/style.css'
                ],
                dest: '<%= dirs.dest %>/assets/css/pack.css'
            },
            mainJs: {
                src: ['<%= dirs.src %>/assets/js/vendor/plugins.js',
                      '<%= dirs.src %>/assets/js/no-js-class.js',
                      '<%= dirs.src %>/assets/js/onLoad.js',
                      '<%= dirs.src %>/assets/js/sf-accel.js',
                      '<%= dirs.src %>/assets/js/google-analytics.js'
                ],
                dest: '<%= dirs.dest %>/assets/js/main.js'
            },
            baguetteBox: {
                src: ['<%= dirs.src %>/assets/js/vendor/baguetteBox.js',
                      '<%= dirs.src %>/assets/js/baguetteBox-init.js'
                ],
                dest: '<%= dirs.dest %>/assets/js/baguetteBox-pack.js'
            }
        },

        autoprefixer: {
            options: {
                browsers: [
                    'last 2 version',
                    '> 1%',
                    'Edge >= 12',
                    'Explorer >= 9',
                    'Firefox ESR',
                    'Opera 12.1'
                ]
            },
            pack: {
                src: '<%= concat.css.dest %>',
                dest: '<%= concat.css.dest %>'
            }
        },

        uncss: {
            options: {
                htmlroot: '<%= dirs.dest %>',
                ignore: [
                    /(#|\.)baguetteBox(-[a-zA-Z]+)?/,
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
                    compatibility: 'ie9',
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
                    '<%= concat.baguetteBox.dest %>': '<%= concat.baguetteBox.dest %>',
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
                    '<%= dirs.dest %>/assets/js/**/*.js'
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
                base: '<%= dirs.dest %>/',
                hostname: 'localhost'
            },
            livereload: {
                options: {
                    base: '<%= dirs.dest %>/',
                    livereload: 35729,
                    open: true,  // Automatically open the webpage in the default browser
                    port: 8000
                }
            },
            linkChecker: {
                options: {
                    base: '<%= dirs.dest %>/',
                    port: 9001
                }
            }
        },

        watch: {
            options: {
                livereload: '<%= connect.livereload.options.livereload %>'
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

        linkChecker: {
            options: {
                callback: function (crawler) {
                    crawler.addFetchCondition(function (queueItem) {
                        return !queueItem.path.match(/\/docs\/(release|nightly)\//) &&
                               queueItem.path !== '/assets/js/vendor/g.src' &&
                               queueItem.path !== '/assets/js/+f+' &&
                               queueItem.path !== '/assets/js/%7Bhref%7D' &&
                               queueItem.path !== '/a';
                    });
                },
                interval: 1,        // 1 ms; default 250
                maxConcurrency: 5   // default; bigger doesn't seem to improve time
            },
            dev: {
                site: 'localhost',
                options: {
                    initialPort: '<%= connect.linkChecker.options.port %>'
                }
            }
        },

        clean: {
            dist: '<%= dirs.dest %>/'
        }

    });

    // Load any grunt plugins found in package.json.
    require('load-grunt-tasks')(grunt, { scope: 'devDependencies' });
    require('time-grunt')(grunt);

    grunt.registerTask('build', [
        'clean',
        'jekyll',
        'useminPrepare',
        'concat',
        'autoprefixer',
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
        'htmllint',
        'connect:linkChecker',
        'linkChecker'
    ]);

    grunt.registerTask('dev', [
        'jekyll',
        'useminPrepare',
        'concat',
        'autoprefixer',
        'filerev',
        'usemin'
    ]);

    grunt.registerTask('server', [
        'build',
        'connect:livereload',
        'watch:build'
    ]);

    grunt.registerTask('default', [
        'dev',
        'connect:livereload',
        'watch:dev'
    ]);

};
